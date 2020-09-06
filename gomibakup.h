/*
 * gomibakup.h
 *
 */

#ifndef GOMIBAKUP_H_
#define GOMIBAKUP_H_

void read_prg_f(uint16_t a, uint16_t b);
void request_dump(void);
void reset_script(void);
void store_command(bool read, uint16_t a, uint16_t val);
void execute_script(void);
void start_thread(void);
void hw_io_prg(bool read);
void read_prg_f(uint16_t addr, uint16_t size);
void write_prg(uint16_t addr, uint8_t val);
void fill_command(uint16_t size, bool *action, uint16_t *addr, uint16_t *val);

#endif /* GOMIBAKUP_H_ */
