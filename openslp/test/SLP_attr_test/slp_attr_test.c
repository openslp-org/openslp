
#include <slp.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <libslpattr.h>

#define STR "a string of length."

#define STR1 "hi \\there again"
#define STR2 "yet another value"

#define OP1 "aasdf"
#define OP2 "ae\nl;adsf;\\y"

#define SER1 "(abool=false),keyw,(anInt=51)"

#define TERM_INT  (int)-2345

#include <stdarg.h>

/* Tests a serialized string to see if an attribute contains the named 
 * values. 
 *
 * Params:
 * str - The serialized attribute list. 
 * name - Name of the attribute to test. 
 * ... - List of strings the attribute must contain. Null terminated. 
 * 
 * Returns 1 iff the attributes contain all of the named values. 0 otherwise.
 */
int test_serialized(char *str, char *name, ...) {
	char *vl_start; /* The start of the serialized value list. */
	char *vl_end; /* End of the value list. The ')'. */
	int vl_size; /* The expected size of the value list. */

	va_list ap;
	char *current_value; /* Current member of ap. */
	
	/***** Find value list. *****/
	{
		char *attr_name; /* The attribute name as it should appear in the serialized string. (ie, with preceeding '(' and trailing '=')*/
		
		int len; /* The length of the name. */
		
		/**** Build attribute as it should appear in str. ****/
		len = strlen(name) + 2; /* +2 for '(' and '='. */

		attr_name = (char *)malloc(len + 1); /* +1 for null. */
		assert(attr_name);

		/*** Add pre/post fix. ***/
		*attr_name = '(';
		strcpy(attr_name + 1, name);
		attr_name[len - 1] = '=';
		attr_name[len] = 0;

		
		/**** Search in attribute list. ****/
		vl_start = strstr(str, attr_name);
		len = strlen(attr_name); /* Icky variable reuse. */
		free(attr_name);
		if (vl_start == NULL) {
			return 0;
		}

		vl_start += len; /* Zip to the start of the attribute list. */
		vl_end = strchr(vl_start, ')');
		if (vl_end == NULL) {
			return 0; /* No terminating ')'. Syntax error. */
		}
	}

	vl_size = 0;
	
	/***** Text contents of value list. *****/
	va_start(ap, name);
	current_value = va_arg(ap, char *);

	while (current_value != NULL) { /* Foreach value... */
		char *index; /* Index to current_value in value list. */
		
		/* Check to see if the current value is in the value list. */
		index = strstr(vl_start, current_value);
		if (index == NULL) {
			/* A value is missing. */
			va_end(ap);
			return 0;
		}

		/* Check that it's _within_ the list. */
		if (index > vl_end) {
			va_end(ap);
			return 0;
		}

		/* TODO Check that it is properly delimited. */
		
		
		/* Increment vl_size. */
		vl_size += strlen(current_value) + 1; /* +1 for comma. */
		
		current_value = va_arg(ap, char *); /* NEXT! */
	}
	va_end(ap);

	
	/***** Check that all values in str are accounted for (ie, there are no extra). *****/
	/**** Account for commas in value list. ****/
	vl_size -= 1; /* -1 for lack of trailing comma. */
	if (vl_size != (int) (vl_end - vl_start)) {
		return 0;
	}
	
	return 1;	
}



/* Finds the number of attributes in an attribute list. 
 *
 * Returns the number of attributes in the passed list. 
 */
int test_size(SLPAttributes attr) {
	SLPAttrIterator iter;
	SLPError err;
	char const *tag; /* The tag. */
	SLPType type; 
	int count;
	
	err = SLPAttrIteratorAlloc(attr, &iter);

	count = 0;
	while (SLPAttrIterNext(iter, &tag, &type) == SLP_TRUE) {
		count++;
	}

	SLPAttrIteratorFree(iter);
	
	return count;
}


/* Tests the named attribute to see if it has all of the named values. 
 *
 * Params:
 * name - Name of the attribute to test. 
 * ... - List of ints that must be associated with name. Terminated with a 
 * 		TERM_INT.
 * 
 * Returns 1 iff the attributes contain all of the named values. 0 otherwise.
 */
int test_int(SLPAttributes attr, char *name, ...) {
	int *int_arr; /* Values returned from SLPAttrGet_str(). */
	int len; /* Number of values returned. */
	int current_value; /* The passed value currently being tested. */
	int values_seen; /* The count of passed values. */
	int i;
	va_list ap;
	SLPError err;

	err = SLPAttrGet_int(attr, name, &int_arr, &len); 
	assert(err == SLP_OK);

	/* Foreach value... */
	values_seen = 0;
	va_start(ap, name);
	current_value = va_arg(ap, int);
	while (current_value != TERM_INT) {
		values_seen++;
		/* Check to see if the current value is in the value list. */
		for (i = 0; i < len; i++) {
			if (int_arr[i] == current_value) {
				break;
			}
		}
		
		/* The current value was not found. */
		if (i == len) {
			/* TODO cleanup:  */
			va_end(ap);
			return 0;
		}
		
		current_value = va_arg(ap, int);
	}
	va_end(ap);

	/* Cleanup memory. */
	free(int_arr);
	
	/* Check that all values in str_attr are accounted for (ie, there are no extra). */
	if (values_seen != len) {
		return 0;
	}
	
	return 1;	
}

/* Tests the named attribute to see if it has all of the named attributes. 
 *
 * Params:
 * name - Name of the attribute to test. 
 * ... - List of strings that must be associated with name. Terminated with a 
 * 		NULL.
 * 
 * Returns 1 iff the attributes contain all of the named values. 0 otherwise.
 */
int test_string(SLPAttributes attr, char *name, ...) {
	char **str_arr; /* Values returned from SLPAttrGet_str(). */
	int len; /* Number of values returned. */
	char *current_value; /* The passed value currently being tested. */
	int values_seen; /* The count of passed values. */
	int i;
	va_list ap;
	SLPError err;

	err = SLPAttrGet_str(attr, name, &str_arr, &len); 
	assert(err == SLP_OK);

	/* Foreach value... */
	values_seen = 0;
	va_start(ap, name);
	current_value = va_arg(ap, char *);
	while (current_value != NULL) {
		values_seen++;
		/* Check to see if the current value is in the value list. */
		for (i = 0; i < len; i++) {
			if (strcmp(current_value, str_arr[i]) == 0) {
				break;
			}
		}
		
		/* The current value was not found. */
		if (i == len) {
			/* TODO cleanup:  */
			va_end(ap);
			return 0;
		}
		
		current_value = va_arg(ap, char *);
	}
	va_end(ap);

	/* Cleanup memory. */
	for (i = 0; i < len; i++) {
		free(str_arr[i]);
	}
	free(str_arr);
	
	/* Check that all values in str_attr are accounted for (ie, there are no extra). */
	if (values_seen != len) {
		return 0;
	}
	
	return 1;	
}

/* Forward declare. */
int find_value_list_end(char *value, int *value_count, SLPType *type, int *unescaped_len, char **end);

int main(int argc, char *argv[]) {
	SLPAttributes attr;
	SLPError err;
	char *str, *str2;
	int len;

#ifdef ENABLE_PREDICATES	
	char data[] = STR;
	SLPBoolean bool;
	int *int_arr;
	char **str_arr;
	SLPType type;
	char *tag;
	SLPAttrIterator iter;
	
	len = strlen(data);
	data[4] = '\0';
	
	printf("Predicates enabled. Performing full test for libslpattr.c\n");
	
	/***** Test string parsing. *****/
	{
		int int_err; /* Somewhere to store integer return values. */
		int value_count; 
		SLPType type;
		int unescaped_len;
		char *cur;
		char *val;

		/*** Valid case: Single string. ***/
		val = "value";
		int_err = find_value_list_end(val, &value_count, &type, &unescaped_len, &cur);
		assert(int_err == 1);
		assert(value_count == 1);
		assert(type == SLP_STRING);
		assert(unescaped_len == strlen(val));
		assert(cur == val + (strlen(val)));


		/*** Valid case: paired string. ***/
		val = "value,12";
		int_err = find_value_list_end(val, &value_count, &type, &unescaped_len, &cur);
		assert(int_err == 1);
		assert(value_count == 2);
		assert(type == SLP_STRING);
		assert(unescaped_len == strlen(val) - 1);
		assert(cur == val + (strlen(val)));


		/*** Valid case: tripled string. ***/
		val = "2,value,12";
		int_err = find_value_list_end(val, &value_count, &type, &unescaped_len, &cur);
		assert(int_err == 1);
		assert(value_count == 3);
		assert(type == SLP_STRING);
		assert(unescaped_len == strlen(val) - 2);
		assert(cur == val + (strlen(val)));


		/*** Valid case: tripled string. ***/
		val = "2,v,12";
		int_err = find_value_list_end(val, &value_count, &type, &unescaped_len, &cur);
		assert(int_err == 1);
		assert(value_count == 3);
		assert(type == SLP_STRING);
		assert(unescaped_len == strlen(val) - 2);
		assert(cur == val + (strlen(val)));


		/*** Valid case: boolean. ***/
		val = "true";
		int_err = find_value_list_end(val, &value_count, &type, &unescaped_len, &cur);
		assert(int_err == 1);
		assert(value_count == 1);
		assert(type == SLP_BOOLEAN);
		assert(cur == val + (strlen(val)));


		/*** Valid case: boolean. ***/
		val = "false";
		int_err = find_value_list_end(val, &value_count, &type, &unescaped_len, &cur);
		assert(int_err == 1);
		assert(value_count == 1);
		assert(type == SLP_BOOLEAN);
		assert(cur == val + (strlen(val)));


		/*** Valid case: paired string (devolve from int to str). ***/
		val = "false,true";
		int_err = find_value_list_end(val, &value_count, &type, &unescaped_len, &cur);
		assert(int_err == 1);
		assert(value_count == 2);
		assert(type == SLP_STRING);
		assert(unescaped_len == strlen(val) - 1);
		assert(cur == val + (strlen(val)));
		
		
		/*** Valid case: Single opaque. ***/
		val = "\\00\\AB";
		int_err = find_value_list_end(val, &value_count, &type, &unescaped_len, &cur);
		assert(int_err == 1);
		assert(value_count == 1);
		assert(type == SLP_OPAQUE);
		assert(unescaped_len == 1);
		assert(cur == val + (strlen(val)));
		
		
		/*** Valid case: paired opaque. ***/
		val = "\\00\\AB,\\00\\12";
		int_err = find_value_list_end(val, &value_count, &type, &unescaped_len, &cur);
		assert(int_err == 1);
		assert(value_count == 2);
		assert(type == SLP_OPAQUE);
		assert(unescaped_len == 2);
		assert(cur == val + (strlen(val)));
		
		
		/*** Invalid case: Opaque and non-opaque. ***/
		val = "\\00\\AB,\\00\\12,2";
		int_err = find_value_list_end(val, &value_count, &type, &unescaped_len, &cur);
		assert(int_err == 0);
		
		
		/*** Invalid case: Opaque and non-opaque. ***/
		val = "dog,\\00\\AB";
		int_err = find_value_list_end(val, &value_count, &type, &unescaped_len, &cur);
		assert(int_err == 0);
	}
	/***** Test Simple boolean set and get. *****/
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &attr);
	assert(err == SLP_OK);
	
	err = SLPAttrSet_bool(attr, "shouldBeFalse", SLP_FALSE); /* Create. */
	assert(err == SLP_OK);
	
	err = SLPAttrGetType(attr, "shouldBeFalse", &type);
	assert(err == SLP_OK);
	assert(type == SLP_BOOLEAN);
	
	err = SLPAttrGet_bool(attr, "shouldBeFalse", &bool); /* Test. */
	assert(err == SLP_OK);
	assert(bool == SLP_FALSE);

	SLPAttrFree(attr);



	/***** Test simple string set and get. *****/
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &attr);
	assert(err == SLP_OK);

	/**** Create. ****/
	err = SLPAttrSet_str(attr, "str", STR, SLP_ADD); 
	assert(err == SLP_OK);

	err = SLPAttrGetType(attr, "str", &type);
	assert(err == SLP_OK);
	assert(type == SLP_STRING);
	
	/*** Test. ***/
	err = SLPAttrGet_str(attr, "str", &str_arr, &len); 
	assert(err == SLP_OK);
	assert(len == 1);
	assert(strcmp(str_arr[0], STR) == 0);
	free(str_arr[0]);
	free(str_arr);
	
	/**** Append. ****/
	err = SLPAttrSet_str(attr, "str", STR1, SLP_ADD); 
	assert(err == SLP_OK);

	err = SLPAttrGetType(attr, "str", &type);
	assert(err == SLP_OK);
	assert(type == SLP_STRING);
	
	/*** Test. ***/
	err = SLPAttrGet_str(attr, "str", &str_arr, &len); 
	assert(err == SLP_OK);
	assert(len == 2);
	
	if (strcmp(str_arr[0], STR) == 0) {
		assert(strcmp(str_arr[1], STR1) == 0);
	} 
	else {
		assert(strcmp(str_arr[0], STR1) == 0);
		assert(strcmp(str_arr[1], STR) == 0);
	}
	
	free(str_arr[0]);
	free(str_arr[1]);
	free(str_arr);

	/**** Replace. ****/
	err = SLPAttrSet_str(attr, "str", STR2, SLP_REPLACE); 
	assert(err == SLP_OK);

	err = SLPAttrGetType(attr, "str", &type);
	assert(err == SLP_OK);
	assert(type == SLP_STRING);
	
	/*** Test. ***/
	err = SLPAttrGet_str(attr, "str", &str_arr, &len); 
	assert(err == SLP_OK);
	assert(len == 1);
	assert(strcmp(str_arr[0], STR2) == 0);
	free(str_arr[0]);
	free(str_arr);

	SLPAttrFree(attr);


	/***** Test ints. *****/
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &attr);
	assert(err == SLP_OK);
	
	/**** Check an int. ****/
	err = SLPAttrSet_int(attr, "anInt", (int)234, SLP_ADD);
	assert(err == SLP_OK);

	err = SLPAttrGet_int(attr, "anInt", &int_arr, &len);
	assert(err == SLP_OK);
	assert(len == 1);
	assert(int_arr[0] == 234);
	free(int_arr);
	
	/**** Check a bunch of ints. ****/
	err = SLPAttrSet_int(attr, "anInt", (int)345, SLP_ADD);
	assert(err == SLP_OK);

	err = SLPAttrGet_int(attr, "anInt", &int_arr, &len);
	assert(err == SLP_OK);
	assert(len == 2);
	if (int_arr[0] == 234) {
		assert(int_arr[1] == 345);
	}
	else {
		assert(int_arr[0] == 345);
		assert(int_arr[1] == 234);
	}
	free(int_arr);
	
	/**** Check replacement. ****/
	err = SLPAttrSet_int(attr, "anInt", (int)567, SLP_REPLACE);
	assert(err == SLP_OK);

	err = SLPAttrGet_int(attr, "anInt", &int_arr, &len);
	assert(err == SLP_OK);
	assert(len == 1);
	assert(int_arr[0] == 567);
	free(int_arr);
	
	SLPAttrFree(attr);

	/**** Check deserialization. ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, "(doc=12,34,56)");
	assert(err == SLP_OK);
	
	assert(test_int(attr, "doc", (int)12, (int)34, (int)56, TERM_INT));
	
	SLPAttrFree(attr);

	
	/****** Test keyword set and get. *****/
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &attr);
	assert(err == SLP_OK);
	
	err = SLPAttrSet_keyw(attr, "keyw1"); /* Create. */
	assert(err == SLP_OK);
	
	err = SLPAttrGet_keyw(attr, "keyw1"); /* Test. */
	assert(err == SLP_OK);

	err = SLPAttrGet_keyw(attr, "keyw2"); /* Test. */
	assert(err == SLP_TAG_ERROR);

	SLPAttrFree(attr);


	/***** FIXME INSERT OPAQUE TESTING HERE. *****/


	/***** Test guessing facilities. *****/
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &attr);
	assert(err == SLP_OK);

	/**** Single value. ****/
	err = SLPAttrSet_guess(attr, "tag", "string", SLP_REPLACE);
	assert(err == SLP_OK);

	err = SLPAttrGetType(attr, "tag", &type);
	assert(err == SLP_OK);
	assert(type == SLP_STRING);

	err = SLPAttrGet_str(attr, "tag", &str_arr, &len); 
	assert(err == SLP_OK);
	assert(len == 1);
	
	assert(strcmp(str_arr[0], "string") == 0);
	free(str_arr[0]);
	free(str_arr);
	
	/**** Multi value. ****/
	err = SLPAttrSet_guess(attr, "tag", "str1,str2", SLP_REPLACE);
	assert(err == SLP_OK);

	err = SLPAttrGetType(attr, "tag", &type);
	assert(err == SLP_OK);
	assert(type == SLP_STRING);

	err = SLPAttrGet_str(attr, "tag", &str_arr, &len); 
	assert(err == SLP_OK);
	assert(len == 2);
	
//	assert(strcmp(str_arr[0], "string") == 0);
	free(str_arr[0]);
	free(str_arr[1]);
	free(str_arr);
	SLPAttrFree(attr);
	
	
	/**** Test replacement of types. ****/
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &attr);
	assert(err == SLP_OK);
	
	err = SLPAttrSet_keyw(attr, "keyw1"); /* Create. */
	assert(err == SLP_OK);
	
	err = SLPAttrGet_keyw(attr, "keyw1"); /* Test. */
	assert(err == SLP_OK);

	err = SLPAttrSet_bool(attr, "keyw1", SLP_TRUE); /* Create. */
	assert(err == SLP_OK);
	
	err = SLPAttrGet_keyw(attr, "keyw1"); /* Test. */
	assert(err == SLP_TYPE_ERROR);
	err = SLPAttrGet_bool(attr, "keyw1", &bool);
	assert(err == SLP_OK);
	assert(bool == SLP_TRUE);

	err = SLPAttrSet_int(attr, "keyw1", (int)1, SLP_ADD);
	assert(err == SLP_TYPE_ERROR);
	
	SLPAttrFree(attr);

	/**** Maintain two strings simultainiously. ****/
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &attr);
	assert(err == SLP_OK);

	/**** Create. ****/
	err = SLPAttrSet_str(attr, "str", STR, SLP_ADD); 
	assert(err == SLP_OK);
	
	err = SLPAttrSet_str(attr, "str1", STR1, SLP_ADD); 
	assert(err == SLP_OK);

	/*** Test. ***/
	err = SLPAttrGet_str(attr, "str", &str_arr, &len); 
	assert(err == SLP_OK);
	assert(len == 1);
	assert(strcmp(str_arr[0], STR) == 0);
	free(str_arr[0]);
	free(str_arr);
	
	err = SLPAttrGet_str(attr, "str1", &str_arr, &len); 
	assert(err == SLP_OK);
	assert(len == 1);

	assert(strcmp(str_arr[0], STR1) == 0);
	free(str_arr[0]);
	free(str_arr);
	
	err = SLPAttrGet_str(attr, "str", &str_arr, &len); 
	assert(err == SLP_OK);
	assert(len == 1);

	assert(strcmp(str_arr[0], STR) == 0);
	free(str_arr[0]);
	free(str_arr);
	
	SLPAttrFree(attr);

	
	/**** Allocate from string. ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, "(abool=false),keyw,(anInt=51,3)");
	assert(err == SLP_OK);

	err = SLPAttrGetType(attr, "abool", &type);
	assert(err == SLP_OK);
	assert(type == SLP_BOOLEAN);

	err = SLPAttrGet_bool(attr, "abool", &bool);
	assert(err == SLP_OK);
	assert(bool == SLP_FALSE);

	err = SLPAttrGetType(attr, "keyw", &type);
	assert(err == SLP_OK);
	assert(type == SLP_KEYWORD);

	err = SLPAttrGet_keyw(attr, "keyw"); 
	assert(err == SLP_OK);

	err = SLPAttrGetType(attr, "anInt", &type);
	assert(err == SLP_OK);
	assert(type == SLP_INTEGER);

	err = SLPAttrGet_int(attr, "anInt", &int_arr, &len); 
	assert(err == SLP_OK);
	assert(len == 2);

	assert((int_arr[0] == 51 && int_arr[1] == 3) || (int_arr[0] == 3 && int_arr[1] == 51));

	free(int_arr);

	/*** Iterate across the thing. ***/
	err = SLPAttrIteratorAlloc(attr, &iter);
	assert(err == SLP_OK);

	while(SLPAttrIterNext(iter, (char const **)&tag, &type)== SLP_FALSE) {
	}
	SLPAttrIteratorFree(iter);

	str = NULL;
	err = SLPAttrSerialize(attr, NULL, &str, 0, NULL, SLP_TRUE);
	assert(err == SLP_OK);
	
	/* FIXME Check str. */
	
	free(str);
	SLPAttrFree(attr);

	/**** Test freshen(). ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, "(abool=false),keyw,(anInt=51)");
	assert(err == SLP_OK);

	/*** Append without replacing. ***/
	err = SLPAttrFreshen(attr, "(cat=meow)");
	assert(err == SLP_OK);

	err = SLPAttrGet_str(attr, "cat", &str_arr, &len); 
	assert(err == SLP_OK);
	assert(len == 1);

	assert(strcmp(str_arr[0], "meow") == 0);
	free(str_arr[0]);
	free(str_arr);
	
	/*** Replace. ***/
	err = SLPAttrFreshen(attr, "(abool=true)");
	assert(err == SLP_OK);

	err = SLPAttrGet_bool(attr, "abool", &bool);
	assert(err == SLP_OK);
	assert(bool == SLP_TRUE);
	
	SLPAttrFree(attr);

	/**** Test the buffering for serialization. ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, SER1);
	assert(err == SLP_OK);

	/*** Try with too small a buffer.  ***/
	str2 = str = (char *)malloc(5);
	assert(str);
	assert(5 < strlen(SER1) + 1);
	err = SLPAttrSerialize(attr, NULL, &str, 5, NULL, SLP_FALSE);
	assert(str == str2); /* Ensure a new buffer wasn't created. */
	assert(err == SLP_BUFFER_OVERFLOW);
	free(str);

	/*** Try with a "just right" buffer. ***/
	str2 = str = (char *)malloc(strlen(SER1) + 1);
	assert(str);
	err = SLPAttrSerialize(attr, NULL, &str, strlen(SER1)+1, NULL, SLP_FALSE);
	assert(err == SLP_OK);
	assert(str == str2); /* Ensure a new buffer wasn't created. */
	assert(strstr(str, "(abool=false)") != NULL);
	assert(strstr(str, "keyw") != NULL);
	assert(strstr(str, "(anInt=51)") != NULL);
	assert(strlen(str) == 29);
	free(str);

	/*** Try with an "n-1" buffer. (too small) ***/
	str2 = str = (char *)malloc(strlen(SER1));
	assert(str);
	err = SLPAttrSerialize(attr, NULL, &str, strlen(SER1), NULL, SLP_FALSE);
	assert(err == SLP_BUFFER_OVERFLOW);
	free(str);

	SLPAttrFree(attr);

	/**** Test the freshening. ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, "(abool=false),keyw,(anInt=51)");
	assert(err == SLP_OK);

	str = NULL;
	err = SLPAttrSerialize(attr, NULL, &str, 0, NULL, SLP_TRUE);
	assert(err == SLP_OK);
	assert(strstr(str, "(abool=false)") != NULL);
	assert(strstr(str, "keyw") != NULL);
	assert(strstr(str, "(anInt=51)") != NULL);
	assert(strlen(str) == 29);
	free(str);
	
	err = SLPAttrSet_bool(attr, "abool", SLP_FALSE);
	assert(err == SLP_OK);

	str = NULL;
	err = SLPAttrSerialize(attr, NULL, &str, 0, NULL, SLP_TRUE);
	assert(err == SLP_OK);
	assert(strcmp("(abool=false)", str) == 0);
	free(str);
	
	SLPAttrFree(attr);

	/**** Test serialization. ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, "(a=1)");
	assert(err == SLP_OK);

	str = NULL;
	err = SLPAttrSerialize(attr, NULL, &str, 0, NULL, SLP_TRUE);
	assert(err == SLP_OK);
	assert(strcmp(str, "(a=1)") == 0);
	free(str);
	
	SLPAttrFree(attr);

	
	/**** Test serialization. ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, "(a=0)");
	assert(err == SLP_OK);

	str = NULL;
	err = SLPAttrSerialize(attr, NULL, &str, 0, NULL, SLP_TRUE);
	assert(err == SLP_OK);
	assert(strcmp(str, "(a=0)") == 0);
	free(str);
	
	SLPAttrFree(attr);

	/**** Test serialization. ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, "(a=-1)");
	assert(err == SLP_OK);

	str = NULL;
	err = SLPAttrSerialize(attr, NULL, &str, 0, NULL, SLP_TRUE);
	assert(err == SLP_OK);
	assert(strcmp(str, "(a=-1)") == 0);
	free(str);
	
	SLPAttrFree(attr);

	/**** Test serialization. ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, "(a=52)");
	assert(err == SLP_OK);

	str = NULL;
	err = SLPAttrSerialize(attr, NULL, &str, 0, NULL, SLP_FALSE);
	assert(err == SLP_OK);
	assert(strcmp(str, "(a=52)") == 0);
	free(str);
	
	SLPAttrFree(attr);

	/**** Test serialization. ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, "(a=-2000)");
	assert(err == SLP_OK);

	str = NULL;
	err = SLPAttrSerialize(attr, NULL, &str, 0, NULL, SLP_FALSE);
	assert(err == SLP_OK);
	assert(strcmp(str, "(a=-2000)") == 0);
	free(str);
	
	SLPAttrFree(attr);

	/**** Do it again. ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, "(public=log),(slug=pig),(doc=12,34,56),zoink");
	assert(err == SLP_OK);
	
	assert(test_int(attr, "doc", (int)12, (int)34, (int)56, TERM_INT));

	/*** Test singular desrialization. ***/
	str = NULL;
	err = SLPAttrSerialize(attr, "doc", &str, 0, NULL, SLP_FALSE);
	assert(err == SLP_OK);

	/* Check that serialized values are there. */
	assert(strstr(str, "12") != NULL);
	assert(strstr(str, "34") != NULL);
	assert(strstr(str, "56") != NULL);
	assert(strstr(str, "doc") != NULL);
	
	/* Check that other tags aren't. */
	assert(strstr(str, "public") == NULL);
	assert(strstr(str, "slug") == NULL);
	assert(strstr(str, "zoink") == NULL);

	free(str);
	
	/*** Test binary deserialization. ***/
	str = NULL;
	err = SLPAttrSerialize(attr, "doc,zoink", &str, 0, NULL, SLP_FALSE);
	assert(err == SLP_OK);

	/* Check that serialized values are there. */
	assert(strstr(str, "12") != NULL);
	assert(strstr(str, "34") != NULL);
	assert(strstr(str, "56") != NULL);
	assert(strstr(str, "zoink") != NULL);
	assert(strstr(str, "doc") != NULL);

	/* Check that other tags aren't. */
	assert(strstr(str, "public") == NULL);
	assert(strstr(str, "slug") == NULL);

	free(str);


	/*** Test binary deserialization. ***/
	str = NULL;
	err = SLPAttrSerialize(attr, "doc,public", &str, 0, NULL, SLP_FALSE);
	assert(err == SLP_OK);

	/* Check that serialized values are there. */
	assert(strstr(str, "12") != NULL);
	assert(strstr(str, "34") != NULL);
	assert(strstr(str, "56") != NULL);
	assert(strstr(str, "log") != NULL);
	assert(strstr(str, "public") != NULL);

	/* Check that other tags aren't. */
	assert(strstr(str, "slug") == NULL);
	assert(strstr(str, "pig") == NULL);
	assert(strstr(str, "zoink") == NULL);

	free(str);
	
	
	/*** Test binary deserialization. ***/
	str = NULL;
	err = SLPAttrSerialize(attr, "doc,zoink,public", &str, 0, NULL, SLP_FALSE);
	assert(err == SLP_OK);

	/* Check that serialized values are there. */
	assert(strstr(str, "12") != NULL);
	assert(strstr(str, "34") != NULL);
	assert(strstr(str, "56") != NULL);
	assert(strstr(str, "zoink") != NULL);
	assert(strstr(str, "doc") != NULL);
	assert(strstr(str, "public=log") != NULL);

	/* Check that other tags aren't. */
	assert(strstr(str, "slug") == NULL);
	assert(strstr(str, "pig") == NULL);

	free(str);

	/*** Test binary deserialization. ***/
	str = NULL;
	err = SLPAttrSerialize(attr, "doc,zoink,public,slug", &str, 0, NULL, SLP_FALSE);
	assert(err == SLP_OK);

	/* Check that serialized values are there. */
	assert(strstr(str, "12") != NULL);
	assert(strstr(str, "34") != NULL);
	assert(strstr(str, "56") != NULL);
	assert(strstr(str, "zoink") != NULL);
	assert(strstr(str, "doc") != NULL);
	assert(strstr(str, "public=log") != NULL);
	assert(strstr(str, "slug=pig") != NULL);

	assert(strlen(str) == 44);
	
	free(str);
	
	SLPAttrFree(attr);


	/**** Test deserialization. ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, "(attr1=val1),(attr2=val1,val2),(attr3=val1,val2,val3)");
	assert(err == SLP_OK);

	/* Check that all values were deserialized. */
	assert(test_string(attr, "attr1", "val1", NULL));
	assert(test_string(attr, "attr2", "val1", "val2", NULL));
	assert(test_string(attr, "attr3", "val1", "val2", "val3", NULL));

	/* Check that there are no extra values. */
	assert(test_size(attr) == 3);
	
	SLPAttrFree(attr);
	
	
	/**** Test deserialization. ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, "(attr1=val1),(attr2=val1,val2),(attr3=val1,val2,val3)");
	assert(err == SLP_OK);

	/* Check that all values were deserialized. */
	assert(test_string(attr, "attr1", "val1", NULL));
	assert(test_string(attr, "attr2", "val1", "val2", NULL));
	assert(test_string(attr, "attr3", "val1", "val2", "val3", NULL));

	/* Check that there are no extra values. */
	assert(test_size(attr) == 3);
	
	SLPAttrFree(attr);
	
	
	/**** Test deserialization. ****/
	str2 = "keyw1,keyw2,(ValX3=val1,val2,val3),(ints=12,3244,93,-15,2,-135),(aBool1=true),(aBool2=false)";
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, str2);
	assert(err == SLP_OK);

	/* Check that all values were deserialized. */
	assert(SLPAttrGet_keyw(attr, "keyw1") == SLP_OK);
	assert(SLPAttrGet_keyw(attr, "keyw2") == SLP_OK);
	assert(test_string(attr, "ValX3", "val1","val2","val3", NULL));
	assert(test_int(attr, "ints", 12, 3244, 93, -15, 2, -135, TERM_INT));
	assert(SLPAttrGet_bool(attr, "aBool1", &bool) == SLP_OK && bool == SLP_TRUE);
	assert(SLPAttrGet_bool(attr, "aBool2", &bool) == SLP_OK && bool == SLP_FALSE);

	/*** Test standard case of deserialization. ***/
	str = (char *)93; /* It's ok. it shouldn't write here, cause we pass a length of zero in. */
	err = SLPAttrSerialize(attr, NULL, &str, 0, &len, SLP_FALSE);
	assert(err == SLP_BUFFER_OVERFLOW);
	assert(strlen(str2) + 1 == len); /* +1 for null. */

	str = (char *)malloc(len);
	assert(str != NULL);
	/* Check for n+1 error. */
	err = SLPAttrSerialize(attr, NULL, &str, len, &len, SLP_FALSE);
	assert(err == SLP_OK);

	free(str);
	
	SLPAttrFree(attr);

	
	/*** Test that integers/strings are properly recognized. ***/
	str2 = "(int=1,2,3),(str=1,2-3)";
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, str2);
	assert(err == SLP_OK);

	assert(test_string(attr, "str", "1","2-3", NULL));
	assert(test_int(attr, "int", 1, 2, 3, TERM_INT));
	
	SLPAttrFree(attr);
	
#else /* ENABLE_PREDICATES */
	printf("Predicates disabled. Performing partial test for libslpattr_tiny.c");

	
#endif /* ENABLE_PREDICATES */
	/***** Common tests. *****/
	
	/**** Basic creation. ****/
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &attr);
	assert(err == SLP_OK);

	SLPAttrFree(attr);

	/**** Test deserialization. ****/
	str2 = "keyw1,keyw2,(ValX3=val1,val2,val3),(ints=12,3244,93,-15,2,-135),(aBool1=true),(aBool2=false)";
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &attr);
	assert(err == SLP_OK);

	err = SLPAttrFreshen(attr, str2);
	assert(err == SLP_OK);
	
	/*** Test standard case of deserialization. ***/
	str = (char *)93; /* It's ok. it shouldn't write here, cause we pass a length of zero in. */
	err = SLPAttrSerialize(attr, NULL, &str, 0, &len, SLP_FALSE);
	assert(err == SLP_BUFFER_OVERFLOW);
	assert(strlen(str2) + 1 == len); /* +1 for null. */

	str = (char *)malloc(len);
	assert(str != NULL);
	/* Check for n+1 error. */
	err = SLPAttrSerialize(attr, NULL, &str, len, &len, SLP_FALSE);
	assert(err == SLP_OK);

	/* Check length of serialized. */
	assert(strlen(str) == strlen(str2));

	/* Check contents of serialized. */
	assert(strstr(str, "keyw1"));
	assert(strstr(str, "keyw2"));
	assert(strstr(str, "keyw2"));

	free(str);
	
	SLPAttrFree(attr);
	
	/**** Create and freshen. ****/
	str2 = "keyword,(aBool=true),(int=1,2,3,-4,1000,-2000),(str=val 1,val 2,val 3)";
	
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &attr);
	assert(err == SLP_OK);

	err = SLPAttrFreshen(attr, str2);
	assert(err == SLP_OK);

	/*** Check the serialized attribute string. ***/
	assert(test_serialized(str2, "aBool", "true", NULL));
	assert(test_serialized(str2, "int", "1", "2", "3", "-4", "1000", "-2000", NULL));
	assert(test_serialized(str2, "str", "val 1", "val 2", "val 3", NULL));
	
	
	SLPAttrFree(attr);

	
	return 0;
}
