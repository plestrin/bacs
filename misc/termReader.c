#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "termReader.h"
#include "base.h"

#define TERMREADER_SPECIAL_CHAR_EOT 0x04
#define TERMREADER_SPECIAL_CHAR_TAB 0x09
#define TERMREADER_SPECIAL_CHAR_LF 	0x0a
#define TERMREADER_SPECIAL_CHAR_ESC 0x1b
#define TERMREADER_SPECIAL_CHAR_DEL 0x7f

#define TERMREADER_CODE_CURR_PREV 	"\x1b[D"
#define TERMREADER_CODE_CURR_NEXT 	"\x1b[C"
#define TERMREADER_CODE_CURR_PUSH 	"\x1b[s"
#define TERMREADER_CODE_CURR_POP 	"\x1b[u"
#define TERMREADER_CODE_CLEAN_LINE 	"\x1b[K"

static void termReader_remove_last_blank(char* buffer, uint32_t length);

static const uint8_t valid_char[128] = {
	0, /* 0x00	Null char */
	0, /* 0x01	Start of Heading */
	0, /* 0x02	Start of Text */
	0, /* 0x03	End of Text */
	0, /* 0x04	End of Transmission */
	0, /* 0x05	Enquiry */
	0, /* 0x06	Acknowledgment */
	0, /* 0x07	Bell */
	0, /* 0x08	Back Space */
	0, /* 0x09	Horizontal Tab */
	0, /* 0x0A	Line Feed */
	0, /* 0x0B	Vertical Tab */
	0, /* 0x0C	Form Feed */
	0, /* 0x0D	Carriage Return */
	0, /* 0x0E	Shift Out / X-On */
	0, /* 0x0F	Shift In / X-Off */
	0, /* 0x10	Data Line Escape */
	0, /* 0x11	Device Control 1 (oft. XON) */
	0, /* 0x12	Device Control 2 */
	0, /* 0x13	Device Control 3 (oft. XOFF) */
	0, /* 0x14	Device Control 4 */
	0, /* 0x15	Negative Acknowledgement */
	0, /* 0x16	Synchronous Idle */
	0, /* 0x17	End of Transmit Block */
	0, /* 0x18	Cancel */
	0, /* 0x19	End of Medium */
	0, /* 0x1A	Substitute */
	0, /* 0x1B	Escape */
	0, /* 0x1C	File Separator */
	0, /* 0x1D	Group Separator */
	0, /* 0x1E	Record Separator */
	0, /* 0x1F	Unit Separator */
	1, /* 0x20	Space */
	1, /* 0x21	Exclamation mark */
	0, /* 0x22	Double quotes (or speech marks) */
	0, /* 0x23	Number */
	0, /* 0x24	Dollar */
	0, /* 0x25	Procenttecken */
	0, /* 0x26	Ampersand */
	0, /* 0x27	Single quote */
	1, /* 0x28	Open parenthesis (or open bracket) */
	1, /* 0x29	Close parenthesis (or close bracket) */
	0, /* 0x2A	Asterisk */
	1, /* 0x2B	Plus */
	1, /* 0x2C	Comma */
	1, /* 0x2D	Hyphen */
	1, /* 0x2E	Period, dot or full stop */
	1, /* 0x2F	Slash or divide */
	1, /* 0x30	Zero */
	1, /* 0x31	One */
	1, /* 0x32	Two */
	1, /* 0x33	Three */
	1, /* 0x34	Four */
	1, /* 0x35	Five */
	1, /* 0x36	Six */
	1, /* 0x37	Seven */
	1, /* 0x38	Eight */
	1, /* 0x39	Nine */
	1, /* 0x3A	Colon */
	1, /* 0x3B	Semicolon */
	0, /* 0x3C	Less than (or open angled bracket) */
	1, /* 0x3D	Equals */
	0, /* 0x3E	Greater than (or close angled bracket) */
	0, /* 0x3F	Question mark */
	0, /* 0x40	At symbol */
	1, /* 0x41	Uppercase A */
	1, /* 0x42	Uppercase B */
	1, /* 0x43	Uppercase C */
	1, /* 0x44	Uppercase D */
	1, /* 0x45	Uppercase E */
	1, /* 0x46	Uppercase F */
	1, /* 0x47	Uppercase G */
	1, /* 0x48	Uppercase H */
	1, /* 0x49	Uppercase I */
	1, /* 0x4A	Uppercase J */
	1, /* 0x4B	Uppercase K */
	1, /* 0x4C	Uppercase L */
	1, /* 0x4D	Uppercase M */
	1, /* 0x4E	Uppercase N */
	1, /* 0x4F	Uppercase O */
	1, /* 0x50	Uppercase P */
	1, /* 0x51	Uppercase Q */
	1, /* 0x52	Uppercase R */
	1, /* 0x53	Uppercase S */
	1, /* 0x54	Uppercase T */
	1, /* 0x55	Uppercase U */
	1, /* 0x56	Uppercase V */
	1, /* 0x57	Uppercase W */
	1, /* 0x58	Uppercase X */
	1, /* 0x59	Uppercase Y */
	1, /* 0x5A	Uppercase Z */
	1, /* 0x5B	Opening bracket */
	1, /* 0x5C	Backslash */
	1, /* 0x5D	Closing bracket */
	0, /* 0x5E	Caret - circumflex */
	1, /* 0x5F	Underscore */
	0, /* 0x60	Grave accent */
	1, /* 0x61	Lowercase a */
	1, /* 0x62	Lowercase b */
	1, /* 0x63	Lowercase c */
	1, /* 0x64	Lowercase d */
	1, /* 0x65	Lowercase e */
	1, /* 0x66	Lowercase f */
	1, /* 0x67	Lowercase g */
	1, /* 0x68	Lowercase h */
	1, /* 0x69	Lowercase i */
	1, /* 0x6A	Lowercase j */
	1, /* 0x6B	Lowercase k */
	1, /* 0x6C	Lowercase l */
	1, /* 0x6D	Lowercase m */
	1, /* 0x6E	Lowercase n */
	1, /* 0x6F	Lowercase o */
	1, /* 0x70	Lowercase p */
	1, /* 0x71	Lowercase q */
	1, /* 0x72	Lowercase r */
	1, /* 0x73	Lowercase s */
	1, /* 0x74	Lowercase t */
	1, /* 0x75	Lowercase u */
	1, /* 0x76	Lowercase v */
	1, /* 0x77	Lowercase w */
	1, /* 0x78	Lowercase x */
	1, /* 0x79	Lowercase y */
	1, /* 0x7A	Lowercase z */
	1, /* 0x7B	Opening brace */
	0, /* 0x7C	Vertical bar */
	1, /* 0x7D	Closing brace */
	0, /* 0x7E	Equivalency sign - tilde */
	0, /* 0x7F	Delete */
};

int32_t termReader_set_raw_mode(struct termReader* term){
	struct termios settings;

	if (isatty(STDIN_FILENO)){
		term->is_tty = 1;
		if (tcgetattr(STDIN_FILENO, &settings)){
			log_err("tcgetattr fails");
			return -1;
		}
		memcpy(&(term->saved_settings), &settings, sizeof(struct termios));

		settings.c_lflag &= (uint32_t)(~(ICANON | ECHO));

		if (tcsetattr(STDIN_FILENO, TCSANOW, &settings)){
			log_err("tcsetattr fails");
			return -1;
		}
	}
	else{
		term->is_tty = 0;
		#ifdef VERBOSE
		log_warn("stdin is not a terminal - skipping");
		#endif
	}

	return 0;
}

enum ansi_escape_seq{
	ANSI_ESC_PREV,
	ANSI_ESC_NEXT,
	ANSI_ESC_FIRST,
	ANSI_ESC_LAST,
	ANSI_ESC_UP,
	ANSI_ESC_DOWN,
	ANSI_ESC_NONE
};

static enum ansi_escape_seq termReader_get_ansi_escape_sequence(void){
	if (fgetc(stdin) != '['){
		return ANSI_ESC_NONE;
	}

	switch (fgetc(stdin)){
		case '7' : {return ANSI_ESC_FIRST;}
		case '8' : {return ANSI_ESC_LAST;}
		case 'A' : {return ANSI_ESC_UP;}
		case 'B' : {return ANSI_ESC_DOWN;}
		case 'D' : {return ANSI_ESC_PREV;}
		case 'C' : {return ANSI_ESC_NEXT;}
		default  : {return ANSI_ESC_NONE;}
	}
}

int32_t termReader_get_line(struct termReader* term, char* buffer, uint32_t buffer_length){
	uint32_t 	i = 0;
	uint32_t 	j = 0;
	uint32_t 	k = 0;
	int 		character;

	do{
		character = fgetc(stdin);
		switch (character){
			case TERMREADER_SPECIAL_CHAR_EOT 	: {
				return -1;
			}
			case TERMREADER_SPECIAL_CHAR_TAB 	: {
				if (term->is_tty && i == j && term->tab_handler != NULL && term->arg != NULL){
					k = term->tab_handler(buffer, buffer_length, i, term->arg);
					if (k > i){
						#pragma GCC diagnostic ignored "-Wunused-result"
						write(STDIN_FILENO, buffer + i, k - i);
						i = k;
						j = k;
					}
				}
				break;
			}
			case TERMREADER_SPECIAL_CHAR_LF 	: {
				if (term->is_tty){
					#pragma GCC diagnostic ignored "-Wunused-result"
					write(STDIN_FILENO, &character, 1);
				}
				else{
					#pragma GCC diagnostic ignored "-Wunused-result"
					write(STDOUT_FILENO, &character, 1);
				}
				break;
			}
			case TERMREADER_SPECIAL_CHAR_ESC : {
				if (term->is_tty){
					switch (termReader_get_ansi_escape_sequence()){
						case ANSI_ESC_PREV 	: {
							if (j){
								j--;
								#pragma GCC diagnostic ignored "-Wunused-result"
								write(STDIN_FILENO, TERMREADER_CODE_CURR_PREV, sizeof TERMREADER_CODE_CURR_PREV);
							}
							break;
						}
						case ANSI_ESC_NEXT 	: {
							if (j < i){
								j++;
								#pragma GCC diagnostic ignored "-Wunused-result"
								write(STDIN_FILENO, TERMREADER_CODE_CURR_NEXT, sizeof TERMREADER_CODE_CURR_NEXT);
							}
						}
						case ANSI_ESC_FIRST : {
							for ( ; j; j--){
								#pragma GCC diagnostic ignored "-Wunused-result"
								write(STDIN_FILENO, TERMREADER_CODE_CURR_PREV, sizeof TERMREADER_CODE_CURR_PREV);
							}
							break;
						}
						case ANSI_ESC_LAST 	: {
							for ( ; j != i; j++){
								#pragma GCC diagnostic ignored "-Wunused-result"
								write(STDIN_FILENO, TERMREADER_CODE_CURR_NEXT, sizeof TERMREADER_CODE_CURR_NEXT);
							}
							break;
						}
						case ANSI_ESC_UP 	: {
							if (term->history_up_handler != NULL){
								if (term->history_up_handler(buffer, term->arg)){
									for ( ; j; j--){
										#pragma GCC diagnostic ignored "-Wunused-result"
										write(STDIN_FILENO, TERMREADER_CODE_CURR_PREV, sizeof TERMREADER_CODE_CURR_PREV);
									}
									write(STDIN_FILENO, TERMREADER_CODE_CLEAN_LINE, sizeof TERMREADER_CODE_CLEAN_LINE);

									i = strlen(buffer);
									j = i;

									write(STDIN_FILENO, buffer, j);
								}
							}
							break;
						}
						case ANSI_ESC_DOWN 	: {
							if (term->history_down_handler != NULL){
								if (term->history_down_handler(buffer, term->arg)){
									for ( ; j; j--){
										#pragma GCC diagnostic ignored "-Wunused-result"
										write(STDIN_FILENO, TERMREADER_CODE_CURR_PREV, sizeof TERMREADER_CODE_CURR_PREV);
									}
									write(STDIN_FILENO, TERMREADER_CODE_CLEAN_LINE, sizeof TERMREADER_CODE_CLEAN_LINE);

									i = strlen(buffer);
									j = i;

									write(STDIN_FILENO, buffer, j);
								}
							}
							break;
						}
						case ANSI_ESC_NONE 	: {
							break;
						}
					}
				}
				break;
			}
			case TERMREADER_SPECIAL_CHAR_DEL : {
				if (term->is_tty){
					if (j){
						if (j < i){
							memmove(buffer + j - 1, buffer + j, i - j);
							j --;
							i --;

							#pragma GCC diagnostic ignored "-Wunused-result"
							write(STDIN_FILENO, TERMREADER_CODE_CURR_PREV, sizeof TERMREADER_CODE_CURR_PREV);
							write(STDIN_FILENO, TERMREADER_CODE_CURR_PUSH, sizeof TERMREADER_CODE_CURR_PUSH);
							write(STDIN_FILENO, TERMREADER_CODE_CLEAN_LINE, sizeof TERMREADER_CODE_CLEAN_LINE);
							write(STDIN_FILENO, buffer + j, i - j);
							write(STDIN_FILENO, TERMREADER_CODE_CURR_POP, sizeof TERMREADER_CODE_CURR_POP);
						}
						else{
							i --;
							j --;

							#pragma GCC diagnostic ignored "-Wunused-result"
							write(STDIN_FILENO, TERMREADER_CODE_CURR_PREV, sizeof TERMREADER_CODE_CURR_PREV);
							write(STDIN_FILENO, TERMREADER_CODE_CLEAN_LINE, sizeof TERMREADER_CODE_CLEAN_LINE);
						}
					}
				}
				break;
			}
			case EOF	: {
				return - 1;
			}
			default 	: {
				if (valid_char[character & 0x7f]){
					if (term->is_tty){
						if (j < i){
							memmove(buffer + j + 1, buffer + j, i - j);
							buffer[j ++] = (char)character;
							i ++;

							#pragma GCC diagnostic ignored "-Wunused-result"
							write(STDIN_FILENO, TERMREADER_CODE_CURR_PUSH, sizeof TERMREADER_CODE_CURR_PUSH);
							write(STDIN_FILENO, TERMREADER_CODE_CLEAN_LINE, sizeof TERMREADER_CODE_CLEAN_LINE);
							write(STDIN_FILENO, buffer + j - 1, i - j + 1);
							write(STDIN_FILENO, TERMREADER_CODE_CURR_POP, sizeof TERMREADER_CODE_CURR_POP);
							write(STDIN_FILENO, TERMREADER_CODE_CURR_NEXT, sizeof TERMREADER_CODE_CURR_NEXT);
						}
						else{
							buffer[i ++] = (char)character;
							j++;

							#pragma GCC diagnostic ignored "-Wunused-result"
							write(STDIN_FILENO, &character, 1);
						}
					}
					else{
						buffer[i ++] = (char)character;

						#pragma GCC diagnostic ignored "-Wunused-result"
						write(STDOUT_FILENO, &character, 1);
					}
				}
				break;
			}
		}
	} while (character != TERMREADER_SPECIAL_CHAR_LF && i < buffer_length - 1);
	buffer[i] = '\0';

	termReader_remove_last_blank(buffer, i);

	return 0;
}

int32_t termReader_reset_mode(struct termReader* term){
	if (term->is_tty && tcsetattr(STDIN_FILENO, TCSANOW, &(term->saved_settings))){
		log_err("tcsetattr fails");
		return -1;
	}

	return 0;
}

static void termReader_remove_last_blank(char* buffer, uint32_t length){
	while (length > 0 && buffer[length - 1] == ' '){
		buffer[length - 1] = '\0';
		length--;
	}
}
