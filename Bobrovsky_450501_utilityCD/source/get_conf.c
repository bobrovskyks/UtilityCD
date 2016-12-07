#include <errno.h>
#include <fcntl.h>
#include <linux/swab.h>
#include <linux/types.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <stdio.h>

#define SG_DEV "/dev/sg1"

int sg_fd;
__u8 sense_buffer[32];

int send_cmd(__u8 *cmd, __u8 cmdlen, unsigned int direction, __u8 *data,
             __u32 datalen, unsigned int timeout) {
  int k = 0;
  sg_io_hdr_t io_hdr;

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

int get_conf(__u16 f_num) {
  int i;
  __u8 get_conf_cmd[10];

  __u8 data_buff[16];
  __u32 data_length = 0;
  __u16 current_prof = 0; // Current Profile
  __u16 f_code = 0;       // Feature Code

  if (test_unit_ready() < 0) {
    printf("Unit not ready\n");
    exit(-1);
  }

  memset(data_buff, 0, 16);

  memset(get_conf_cmd, 0, 10);
  get_conf_cmd[0] = 0x46; // GET CONFIGURATION
  get_conf_cmd[1] = 2;    // RT= 10b
  get_conf_cmd[8] = 16;

  f_num = __swab16(f_num);
  memcpy((get_conf_cmd + 2), (void *)&f_num, 2);

  if (send_cmd(get_conf_cmd, 10, SG_DXFER_FROM_DEV, data_buff, 16, 20000) < 0)
    return -1;

  memcpy((void *)&data_length, data_buff, 4);
  data_length = __swab32(data_length);
  printf("\nFeature data length - %u\n", data_length);

  memcpy((void *)&current_prof, data_buff + 6, 2);
  current_prof = __swab16(current_prof);
  printf("Current profile - 0x%.4X\n", current_prof);

  memcpy((void *)&f_code, (data_buff + 8), 2);
  f_code = __swab16(f_code);
  printf("Feature Code - 0x%.4X\n", f_code);

  printf("0x%X\n", data_buff[12]);

  return 0;
}

int main() {
  __u16 f_num = 0x0001; // Core Feature
//    __u16 f_num = 0x0037; // CD-RW Media Write Support Feature
//    __u16 f_num = 0x002E; // CD Mastering (SAO, RAW)
//    __u16 f_num = 0x001D; //Multi-Read
//    __u16 f_num = 0x001E; //CD-Read
//    __u16 f_num = 0x0103; //CD Audio External Play

#define CD_SAO 0x002E
#define CD_TAO 0x002D

  if ((sg_fd = open(SG_DEV, O_RDWR)) < 0) {
    perror("open");
    return -1;
  }

  printf("Get CD Mastering (SAO) Feature:");
  f_num = CD_SAO;
  if (get_conf(f_num) < 0)
    printf("Cannot read CD SAO\n");

  printf("\nGet CD TAO Feature:");
  f_num = CD_TAO;
  if (get_conf(f_num) < 0)
    printf("Cannot read CD TAO\n");

  close(sg_fd);
  return 0;
}
