#include <stdlib.h>
#include <stdio.h>

#include "../multiColumn.h"

int main(){
	struct multiColumnPrinter* printer;

	printer = multiColumnPrinter_create(stdout, 5, NULL, NULL, NULL);
	if (printer == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return 0;
	}

	multiColumnPrinter_set_column_size(printer, 0, 2);
	multiColumnPrinter_set_column_size(printer, 1, 10);
	multiColumnPrinter_set_column_size(printer, 2, 3);
	multiColumnPrinter_set_column_size(printer, 3, 10);
	multiColumnPrinter_set_column_size(printer, 4, 12);

 	multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_UINT32);
 	multiColumnPrinter_set_column_type(printer, 1, MULTICOLUMN_TYPE_STRING);
 	multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_INT32);
 	multiColumnPrinter_set_column_type(printer, 3, MULTICOLUMN_TYPE_DOUBLE);
 	multiColumnPrinter_set_column_type(printer, 4, MULTICOLUMN_TYPE_STRING);

	multiColumnPrinter_set_title(printer, 0, "");
	multiColumnPrinter_set_title(printer, 1, "Name");
	multiColumnPrinter_set_title(printer, 2, "Age");
	multiColumnPrinter_set_title(printer, 3, "Grade");
	multiColumnPrinter_set_title(printer, 4, "Observation");

	multiColumnPrinter_print_header(printer);

	multiColumnPrinter_print(printer, 1, "Robert", 12, 1.4, "Correct");
	multiColumnPrinter_print(printer, 2, "Julia", 16, 1.3, "Excellent");
	multiColumnPrinter_print(printer, 3, "Claude", 8, 56.09, "Brilliant");

	multiColumnPrinter_delete(printer);

	return 0;
}