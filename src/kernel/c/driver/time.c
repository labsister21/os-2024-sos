#include "driver/time.h"
#include "cpu/interrupt.h"
#include "cpu/portio.h"
#include "process/scheduler.h"
#include "text/framebuffer.h"
#include <std/string.h>
#include <time.h>

static bool handled = false;
struct TimeRTC startup_time;
struct TimeRTC current_time;

int day_in_month[] = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
void time_handle_timer_interrupt() {
	static unsigned int interrupt_counter = 0;

	if (interrupt_counter == PIT_TIMER_FREQUENCY) {
		// Add one second
		current_time.second += 1;

		if (current_time.second == 60) {
			current_time.second = 0;
			current_time.minute += 1;
		}

		if (current_time.minute == 60) {
			current_time.minute = 0;
			current_time.hour += 1;
		}

		if (current_time.hour == 24) {
			current_time.hour = 0;
			current_time.day += 1;
		}

		bool day_overflow = false;
		if (current_time.month == 2) {
			bool is_leap_year = (current_time.year % 400 == 0) ||
													(current_time.year % 4 == 0 && current_time.year % 100 != 0);

			int day_in_february = 28;
			if (is_leap_year) day_in_february += 1;
			if (day_in_february == current_time.day)
				day_overflow = true;
		} else {
			if (day_in_month[current_time.month] - 1 == current_time.day)
				day_overflow = true;
		}

		if (day_overflow) {
			current_time.month += 1;
			current_time.day = 1;
		}

		if (current_time.month == 12) {
			current_time.year += 1;
		}

		interrupt_counter = 0;
	}
	// framebuffer_write(0, 79, current_time.second % 10 + '0', WHITE, BLACK);
	// framebuffer_write(0, 78, current_time.second / 10 + '0', WHITE, BLACK);
	//
	// framebuffer_write(0, 76, current_time.minute % 10 + '0', WHITE, BLACK);
	// framebuffer_write(0, 75, current_time.minute / 10 + '0', WHITE, BLACK);
	//
	// framebuffer_write(0, 73, current_time.hour / 10 + '0', WHITE, BLACK);
	// framebuffer_write(0, 72, current_time.hour / 10 + '0', WHITE, BLACK);

	interrupt_counter += 1;
};

void enable_rtc_interrupt() {
	__asm__ volatile("cli");

	// Disable NMI, and CMOS register setup
	out(0x70, 0x8b);
	io_wait();
	uint8_t prev = in(0x71);
	out(0x70, 0x8b);
	out(0x71, prev | 0x40);

	// Enable interrupt
	out(PIC1_DATA, in(PIC1_DATA) & ~(1 << IRQ_CASCADE));
	out(PIC2_DATA, in(PIC2_DATA) & ~(1 << (IRQ_CMOS - 8)));

	// Enable NMI
	out(0x70, in(0x70) & 0x7F);
	in(0x71);

	__asm__ volatile("sti");
}

enum {
	cmos_address = 0x70,
	cmos_data = 0x71
};

unsigned char get_RTC_register(int reg) {
	out(cmos_address, reg);
	return in(cmos_data);
}

void handle_rtc_interrupt() {
	startup_time.second = get_RTC_register(0x00);
	startup_time.minute = get_RTC_register(0x02);
	startup_time.hour = get_RTC_register(0x04);
	startup_time.day = get_RTC_register(0x07);
	startup_time.month = get_RTC_register(0x08);
	startup_time.year = get_RTC_register(0x09);

	unsigned char registerB = get_RTC_register(0x0B);

	// Convert BCD to binary values if necessary
	if (!(registerB & 0x04)) {
		startup_time.second = (startup_time.second & 0x0F) + ((startup_time.second / 16) * 10);
		startup_time.minute = (startup_time.minute & 0x0F) + ((startup_time.minute / 16) * 10);
		startup_time.hour = ((startup_time.hour & 0x0F) + (((startup_time.hour & 0x70) / 16) * 10)) | (startup_time.hour & 0x80);
		startup_time.day = (startup_time.day & 0x0F) + ((startup_time.day / 16) * 10);
		startup_time.month = (startup_time.month & 0x0F) + ((startup_time.month / 16) * 10);
		startup_time.year = (startup_time.year & 0x0F) + ((startup_time.year / 16) * 10);
	}

	// Convert 12 hour clock to 24 hour clock if necessary
	if (!(registerB & 0x02) && (startup_time.hour & 0x80)) {
		startup_time.hour = ((startup_time.hour & 0x7F) + 12) % 24;
	}

	// Disable interrupt
	out(PIC2_DATA, in(PIC2_DATA) | (1 << (IRQ_CMOS - 8)));
	handled = true;
}

void setup_time() {
	enable_rtc_interrupt();
	while (!handled) // Wait till finished reading rtc
		asm("hlt");
	memcpy(&current_time, &startup_time, sizeof(struct TimeRTC));
}
