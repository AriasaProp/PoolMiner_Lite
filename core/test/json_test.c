#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "json.h"

#define MAX_PATH 2048
char path[MAX_PATH];
const char *paths[]{
	"food",
	"multidim_arr",
	"random",
	"reddit",
	"rickandmorty",
	"simple",
};

int json_test(const char *data) {
	for(const char *p : paths){
	  snprintf(path, MAX_PATH, "%s/%s.json", data, p)
	  FILE *file = fopen(path, "r");
	  if (!file) {
	    fprintf(stderr, "Expected file \"%s\" not found", path);
	    continue;
	  }
  	fseek(file, 0, SEEK_END);
  	long len = ftell(file);
  	fseek(file, 0, SEEK_SET);
  	char *buffer = malloc(len + 1);

	  if (!buffer) {
	    fprintf(stderr, "Unable to allocate memory for file");
	    fclose(file);
	    return 1;
	  }

	  fread(buffer, 1, len, file);
	  fclose(file);
	  buffer[len] = '\0';

	  clock_t start, end;
	  start = clock();
	  result(json_element) element_result = json_parse(buffer);
	  end = clock();
	
	  printf("Time taken %fs\n", (double)(end - start) / (double)CLOCKS_PER_SEC);
	
	  free((void *)buffer);
	
	  if (result_is_err(json_element)(&element_result)) {
	    typed(json_error) error = result_unwrap_err(json_element)(&element_result);
	    fprintf(stderr, "Error parsing JSON: %s\n", json_error_to_string(error));
	    return 1;
	  }
	  typed(json_element) element = result_unwrap(json_element)(&element_result);
	
	  // json_print(&element, 2);
	  json_free(&element);
	}
  return 0;
}