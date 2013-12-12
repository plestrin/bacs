#ifndef WORKPERCENT_H
#define WORKPERCENT_H

#include <stdint.h>

enum workPercent_accuracy{
	WORKPERCENT_ACCURACY_0,		/* ex: 42% 		*/
	WORKPERCENT_ACCURACY_1,		/* ex: 42.1% 	*/
	WORKPERCENT_ACCURACY_2	 	/* ex: 42.10% 	*/
};

struct workPercent{
	char*						line;
	enum workPercent_accuracy 	accuracy;
	uint64_t 					nb_unit;
	uint64_t					counter;
	uint32_t 					step_counter;
};

struct workPercent* workPercent_create(char* line, enum workPercent_accuracy accuracy, uint64_t nb_unit);
int32_t workPercent_init(struct workPercent* work, char* line, enum workPercent_accuracy accuracy, uint64_t nb_unit);

void workPercent_notify(struct workPercent* work, uint32_t value);

void workPercent_delete(struct workPercent* work);


#endif