#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>
#include <time.h>
#include <string.h>

// Helper to write values to sysfs configuration paths
int write_sysfs(const char *path, const char *value) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open sysfs path");
        return -1;
    }
    if (write(fd, value, strlen(value)) < 0) {
        perror("Failed to write to sysfs path");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int main() {
    // 1. Force the kernel to treat the RTC as a valid hardware wakeup source
    // Without this, the kernel will mask the RTC interrupt right before sleep
    if (write_sysfs("/sys/class/rtc/rtc0/device/power/wakeup", "enabled") < 0) {
        fprintf(stderr, "Warning: Could not enable wakeup source. Hardware may ignore interrupt.\n");
    }

    // 2. Open the physical Real-Time Clock device
    int rtc_fd = open("/dev/rtc0", O_RDONLY);
    if (rtc_fd < 0) {
        perror("Failed to open /dev/rtc0");
        return EXIT_FAILURE;
    }

    // 3. Clear any lingering stale hardware alarms
    if (ioctl(rtc_fd, RTC_AIE_OFF) < 0) {
        // Not a fatal error if no alarm was active
    }

    // 4. Fetch current hardware time (Qualcomm RTCs operate in UTC/GMT)
    struct rtc_time rtc_tm;
    if (ioctl(rtc_fd, RTC_RD_TIME, &rtc_tm) < 0) {
        perror("ioctl(RTC_RD_TIME) failed");
        close(rtc_fd);
        return EXIT_FAILURE;
    }
    printf("Current Hardware RTC Time: %02d:%02d:%02d\n", rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

    // 5. Schedule the wake event exactly 20 seconds into the future
    // Convert to Unix Epoch seconds to handle minute/hour overflows cleanly
    struct tm broken_down;
    memset(&broken_down, 0, sizeof(struct tm));
    broken_down.tm_sec  = rtc_tm.tm_sec;
    broken_down.tm_min  = rtc_tm.tm_min;
    broken_down.tm_hour = rtc_tm.tm_hour;
    broken_down.tm_mday = rtc_tm.tm_mday;
    broken_down.tm_mon  = rtc_tm.tm_mon;
    broken_down.tm_year = rtc_tm.tm_year;
    
    time_t rtc_epoch = timegm(&broken_down); // Force UTC calculation
    rtc_epoch += 20;                         // Add 20 seconds duration
    
    struct tm *alarm_tm = gmtime(&rtc_epoch);

    // 6. Populate the Linux wake-alarm structure
    struct rtc_wkalrm alarm;
    memset(&alarm, 0, sizeof(struct rtc_wkalrm));
    alarm.enabled = 1; // 1 explicitly instructs the PMIC to pull the wake pin high
    alarm.time.tm_sec  = alarm_tm->tm_sec;
    alarm.time.tm_min  = alarm_tm->tm_min;
    alarm.time.tm_hour = alarm_tm->tm_hour;
    alarm.time.tm_mday = alarm_tm->tm_mday;
    alarm.time.tm_mon  = alarm_tm->tm_mon;
    alarm.time.tm_year = alarm_tm->tm_year;

    // Send the alarm structure straight to the PMIC hardware register
    if (ioctl(rtc_fd, RTC_WKALM_SET, &alarm) < 0) {
        perror("ioctl(RTC_WKALM_SET) failed");
        close(rtc_fd);
        return EXIT_FAILURE;
    }
    printf("Hardware wake-alarm successfully armed for 20 seconds from now.\n");

    // Close the file descriptor so the driver isn't held open during suspend
    close(rtc_fd);

    // 7. Put the device into deep suspend-to-RAM state
    printf("Entering deep sleep ('mem')...\n");
    fflush(stdout);
    sync(); // Flush filesystem caches to disk before sleep
    
    if (write_sysfs("/sys/power/state", "mem") < 0) {
        perror("Failed to suspend device");
        return EXIT_FAILURE;
    }

    // 8. Execution resumes exactly HERE once the physical hardware pulse triggers
    printf("\n💥 SYSTEM WOKE UP SUCCESSFULLY VIA HARDWARE INTERRUPT! 💥\n");

    return EXIT_SUCCESS;
}
