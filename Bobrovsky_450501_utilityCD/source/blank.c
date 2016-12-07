#include <errno.h>
#include <fcntl.h>
#include <linux/swab.h>
#include <linux/types.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <stdio.h>

#define NAMEDPIPE_NAME "/tmp/cd"
#define SG_DEV "/dev/sg1"

#define FULL_BLANK 0x00
#define MINIMAL_BLANK 0x01

#define Message1 "Blanking disk...\n"
#define Message2 "Cannot blank disk\n"
#define Message3 "OK\n"
#define Message4 "Unit not ready\n"
#define Step0 "0"
#define Step1 "1"
#define Step2 "2"
#define Step3 "3"
#define Step4 "4"
#define Step5 "5"
#define Step6 "6"
#define Step7 "7"
#define Step8 "8"
#define Step9 "9"
#define Step10 "10"

int fd, len;

int sg_fd;

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
      printf("Sense data: ", io_hdr.status);
      printf("SCSI status=0x%x\n", io_hdr.status);
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

void eject_cd() {
  __u8 start_stop_cmd[6];

  memset(start_stop_cmd, 0, 6);

  if (test_unit_ready() < 0)
    return;

  start_stop_cmd[0] = 0x1B;
  start_stop_cmd[4] = 2;

  send_cmd(start_stop_cmd, 6, SG_DXFER_NONE, NULL, 0, 20000);

  return;
}

int blank(__u8 blank_type) {
  __u8 blank_cmd[12];
  if (test_unit_ready() < 0) {
    write(fd, Message4, strlen(Message4));
    exit(-1);
  }
  memset(blank_cmd, 0, 12);
  blank_cmd[0] = 0xA1;       // BLANK
  blank_cmd[1] = blank_type; //
  write(fd, Step2, strlen(Step2));
  sleep(2);
  if (send_cmd(blank_cmd, 12, SG_DXFER_NONE, NULL, 0, 9600 * 1000) < 0)
    return -1;
  write(fd, Step6, strlen(Step6));
  sleep(2);
  return 0;
}

int main() {
#define BUF_SIZE 4096

  char buffer[BUF_SIZE];
  if ((fd = open(NAMEDPIPE_NAME, O_RDWR)) <= 0) {
    perror("Openfile");
    return 1;
  }
  write(fd, Step0, strlen(Step0));
  sleep(2);
  if ((sg_fd = open(SG_DEV, O_RDWR)) < 0) {
    perror("open");
    return -1;
  }
  write(fd, Step1, strlen(Step1));
  sleep(2);
  if (blank(MINIMAL_BLANK) < 0) {
    sleep(10);
    write(fd, Message2, strlen(Message2));
    sleep(10);
  } else {
    sleep(10);
    write(fd, Message3, strlen(Message3));
    sleep(10);
  }
  eject_cd();
  close(sg_fd);
  sleep(2);
  write(fd, Step10, strlen(Step10));
  return 0;
}
