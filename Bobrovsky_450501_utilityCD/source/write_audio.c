#include <errno.h>
#include <fcntl.h>
#include <linux/cdrom.h>
#include <linux/swab.h>
#include <linux/types.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <stdio.h>

#define SG_DEV "/dev/sg1"
#define WAV_HEADER_SIZE 44

int sg_fd;

typedef struct {
  __u8 page_code : 6;
  __u8 rez : 1;
  __u8 ps : 1;
  __u8 page_length;
  __u8 write_type : 4;
  __u8 test_write : 1;
  __u8 ls_v : 1;
  __u8 BUFE : 1;
  __u8 rez1 : 1;
  __u8 track_mode : 4;
  __u8 copy : 1;
  __u8 FP : 1;
  __u8 multises : 2;
  __u8 dbt : 4;
  __u8 rez2 : 4;
  __u8 link_size;
  __u8 rez3;
  __u8 hac : 6;
  __u8 rez4 : 2;
  __u8 s_format;
  __u8 rez5;
  __u32 packet_size;
  __u16 apl;
  __u8 mcn[16];
  __u8 isrc[16];
  __u32 sh;
} __attribute__((packed)) wpm_t;

wpm_t *wpm;
__u16 mode_page_len = 0;
__u16 data_len = 60;
__u8 data_buff[60];

int send_cmd(__u8 *cmd, __u8 cmdlen, unsigned int direction, __u8 *data,
             __u32 datalen, unsigned int timeout) {
  int k = 0;
  sg_io_hdr_t io_hdr;
  __u8 sense_buffer[32];

  memset(&io_hdr, 0, sizeof(sg_io_hdr_t));

  io_hdr.interface_id = 'S';
  io_hdr.cmd_len = cmdlen;
  io_hdr.mx_sb_len = sizeof(sense_buffer);
  io_hdr.dxfer_direction = direction;
  io_hdr.dxfer_len = datalen;
  io_hdr.dxferp = data;
  io_hdr.cmdp = cmd;
  io_hdr.sbp = sense_buffer;
  io_hdr.timeout = timeout; /* 20000 millisecs == 20 seconds */

  if (ioctl(sg_fd, SG_IO, &io_hdr) < 0) {
    perror("SG_IO ioctl");
    return -1;
  }

  /* now for the error processing */
  if ((io_hdr.info & SG_INFO_OK_MASK) != SG_INFO_OK) {
    if (io_hdr.sb_len_wr > 0) {
      printf("Sense data: ");
      for (k = 0; k < io_hdr.sb_len_wr; ++k) {
        if ((k > 0) && (0 == (k % 10)))
          printf("\n  ");
        printf("0x%02x ", sense_buffer[k]);
      }
      printf("\n");
    }
    if (io_hdr.masked_status)
      printf("SCSI status=0x%x\n", io_hdr.status);
    if (io_hdr.host_status)
      printf("Host_status=0x%x\n", io_hdr.host_status);
    if (io_hdr.driver_status)
      printf("Driver_status=0x%x\n", io_hdr.driver_status);

    return -1;
  }

  return 0;
}

int test_unit_ready() {
  __u8 test_unit_cmd[6];

  /* Prepare TEST UNIT command */
  memset(test_unit_cmd, 0, 6);
  if (send_cmd(test_unit_cmd, 6, SG_DXFER_NONE, NULL, 0, 20000) < 0)
    return -1;

  return 0;
}

int mode_sense() {
  __u8 mode_sense_cmd[10];

  if (test_unit_ready() < 0) {
    printf("Unit not ready\n");
    exit(-1);
  }

  memset(data_buff, 0, 60);
  wpm = (void *)(data_buff + 8);

  /* Prepare MODE_SENSE_10 command */
  memset(mode_sense_cmd, 0, 10);
  mode_sense_cmd[0] = MODE_SENSE_10;
  mode_sense_cmd[2] = 5; 
  mode_sense_cmd[8] = data_len; // 60

  if (send_cmd(mode_sense_cmd, 10, SG_DXFER_FROM_DEV, data_buff, data_len,
               20000) < 0)
    return -1;

  memcpy((void *)&mode_page_len, data_buff, 2);
  mode_page_len = __swab16(mode_page_len);

  printf("Mode data length - %d\n", mode_page_len);
  printf("Page code - %d\n", wpm->page_code);
  printf("Page length - %d (0x%X)\n", wpm->page_length, wpm->page_length);
  printf("Write type - %d\n", wpm->write_type);
  printf("Data block type - %d\n", wpm->dbt);
  printf("Session format - %d\n", wpm->s_format);
  printf("Track mode - %d\n", wpm->track_mode);

  return 0;
}

int mode_select(__u8 dbt, __u8 track_mode) {
  __u8 mode_select_cmd[10];

  wpm->write_type = 1; // 2 - SAO, 1 - TAO, 3 - RAW
  wpm->dbt = dbt;
  wpm->track_mode = track_mode; //!!!!!! CDDA !!!!!!
  wpm->s_format = 0;
  wpm->multises = 0;

  if (test_unit_ready() < 0) {
    printf("Unit not ready\n");
    exit(-1);
  }

  /* Prepare MODE_SELECT_10 command */
  memset(mode_select_cmd, 0, 10);
  mode_select_cmd[0] = MODE_SELECT_10;
  mode_select_cmd[1] = 0x10;

  data_len = 60;
  mode_page_len = __swab16(data_len);
  memcpy((void *)(mode_select_cmd + 7), (void *)&mode_page_len, 2);

  if (send_cmd(mode_select_cmd, 10, SG_DXFER_TO_DEV, data_buff, data_len,
               20000) < 0)
    return -1;

  return 0;
}

__u32 write_audio(__u8 *file_name, __u32 start_lba) {
  int in_f;
  __u8 write_cmd[10];
  __u8 write_buff[CD_FRAMESIZE_RAW];
  __u32 lba = 0, lba1 = 0;

  if (test_unit_ready() < 0) {
    printf("Unit not ready\n");
    exit(-1);
  }

  lba1 = start_lba;

  in_f = open(file_name, O_RDONLY, 0600);
  lseek(in_f, WAV_HEADER_SIZE, 0);
  memset(write_buff, 0, CD_FRAMESIZE_RAW);

  while (read(in_f, write_buff, CD_FRAMESIZE_RAW) > 0) {

    /* Prepare WRITE_10 command */
    memset(write_cmd, 0, 10);
    write_cmd[0] = WRITE_10;
    write_cmd[8] = 1;

    printf("%c", 0x0D);
    printf("lba - %6d", lba1);
    lba = __swab32(lba1);
    memcpy((write_cmd + 2), (void *)&lba, 4);
    lba1 += 1;

    if (send_cmd(write_cmd, 10, SG_DXFER_TO_DEV, write_buff, CD_FRAMESIZE_RAW,
                 20000) < 0)
      return 0;
  }

  printf("\n");

  return lba1;
}

int sync_cache() {
  __u8 flush_cache[10];

  if (test_unit_ready() < 0) {
    printf("Unit not ready\n");
    exit(-1);
  }

  memset(flush_cache, 0, 10);
  flush_cache[0] = SYNCHRONIZE_CACHE;

  if (send_cmd(flush_cache, 10, SG_DXFER_NONE, NULL, 0, 20000) < 0)
    return -1;

  return 0;
}

int close_track(int trk) {
  __u8 close_trk_cmd[10];

  if (sync_cache() < 0) {
    printf("Cannot synchronize cache!\n");
    return -1;
  }

  if (test_unit_ready() < 0) {
    printf("Unit not ready\n");
    exit(-1);
  }

  memset(close_trk_cmd, 0, 10);
  close_trk_cmd[0] = 0x5B;
  close_trk_cmd[2] = 1;
  close_trk_cmd[5] = trk;

  if (send_cmd(close_trk_cmd, 10, SG_DXFER_NONE, NULL, 0, 20000) < 0)
    return -1;

  return 0;
}

int close_session() {
  __u8 close_sess_cmd[10];

  if (test_unit_ready() < 0) {
    printf("Unit not ready\n");
    exit(-1);
  }

  memset(close_sess_cmd, 0, 10);
  close_sess_cmd[0] = 0x5B;
  close_sess_cmd[2] = 2;

  if (send_cmd(close_sess_cmd, 10, SG_DXFER_NONE, NULL, 0, 60000) < 0)
    return -1;

  return 0;
}

void eject_cd() {
  __u8 start_stop_cmd[6];

  memset(start_stop_cmd, 0, 6);

  test_unit_ready();

  start_stop_cmd[0] = 0x1B;
  start_stop_cmd[4] = 2;

  send_cmd(start_stop_cmd, 6, SG_DXFER_NONE, NULL, 0, 20000);

  return;
}

int main(int argc, char **argv) {
  int i = 1;
  __u32 start_lba = 0, total_sectors = 0;
  __u16 apl = 0;

  if (argc == 1) {
    printf("\n\tUsage: write_audio [WAV-files]\n\n");
    return 0;
  }

  if ((sg_fd = open(SG_DEV, O_RDWR)) < 0) {
    perror("open");
    return 1;
  }

  if (mode_sense() < 0) {
    printf("Mode sense error\n");
    return -1;
  }

  apl = __swab16(wpm->apl);
  //    printf("Audio pause length - %d\n", apl);

  if (mode_select(0, 0) < 0) {
    printf("Mode select error\n");
    return 0;
  }

  if (mode_sense() < 0) {
    printf("Mode sense error\n");
    return -1;
  }

  for (i = 1; i < argc; i++) {

    printf("\nWriting track #%d:\n", i);

    if (i > 1)
      start_lba = total_sectors + apl + 2;

    printf("Start sector - %u\n", start_lba);
    total_sectors = write_audio(argv[i], start_lba);
    if (total_sectors == 0) {
      printf("Cannot write %s\n", argv[i]);
      return -1;
    }

    if (close_track(i) < 0) {
      printf("Cannot close track #%d!\n", i);
      return -1;
    }
  }

  printf("\nClose session..");
  if (close_session() < 0) {
    printf("\nCannot close session!\n");
    return -1;
  }
  printf("OK\n\n");

  eject_cd();
  close(sg_fd);

  return 0;
}
