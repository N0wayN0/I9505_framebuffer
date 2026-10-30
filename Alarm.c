#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>
#include <time.h>
#include <string.h>

int main() {
    int fd = open("/dev/rtc0", O_RDONLY);
    if (fd < 0) {
        perror("open /dev/rtc0");
        return 1;
    }

    struct rtc_wkalrm alarm;
    memset(&alarm, 0, sizeof(alarm));
    alarm.enabled = 1; // Arm the interrupt

    // Example: Set alarm for 60 seconds in the future.
    // You need to fill alarm.time with the target date/time.
    // This part is simplified; you typically get current time first.
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    tm_now->tm_sec += 60;
    mktime(tm_now); // Normalize the time structure

    // Copy the target time into the rtc_time struct
    alarm.time.tm_sec = tm_now->tm_sec;
    alarm.time.tm_min = tm_now->tm_min;
    alarm.time.tm_hour = tm_now->tm_hour;
    alarm.time.tm_mday = tm_now->tm_mday;
    alarm.time.tm_mon = tm_now->tm_mon;
    alarm.time.tm_year = tm_now->tm_year;

    if (ioctl(fd, RTC_WKALM_SET, &alarm) == -1) {
        perror("RTC_WKALM_SET failed");
        close(fd);
        return 1;
    }

    printf("Alarm set via ioctl.\n");
    close(fd);
    return 0;
}
