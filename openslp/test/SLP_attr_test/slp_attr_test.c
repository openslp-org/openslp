
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


int main(int argc, char *argv[]) {
	SLPAttributes attr;
	SLPError err;
	SLPBoolean bool;
	char data[] = STR;
	size_t len;
	long *int_arr;
	char **str_arr;
	SLPType type;
	char *str, *tag;
	SLPAttrIterator iter;
	
	len = strlen(data);
	data[4] = '\0';
	


	/****** Test Simple boolean set and get. *****/
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
	err = SLPAttrSet_int(attr, "anInt", (long)234, SLP_ADD);
	assert(err == SLP_OK);

	err = SLPAttrGet_int(attr, "anInt", &int_arr, &len);
	assert(err == SLP_OK);
	assert(len == 1);
	assert(int_arr[0] == 234);
	free(int_arr);
	
	/**** Check a bunch of ints. ****/
	err = SLPAttrSet_int(attr, "anInt", (long)345, SLP_ADD);
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
	err = SLPAttrSet_int(attr, "anInt", (long)567, SLP_REPLACE);
	assert(err == SLP_OK);

	err = SLPAttrGet_int(attr, "anInt", &int_arr, &len);
	assert(err == SLP_OK);
	assert(len == 1);
	assert(int_arr[0] == 567);
	free(int_arr);
	
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

	err = SLPAttrSet_int(attr, "keyw1", (long)1, SLP_ADD);
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

	err = SLPAttrSerialize(attr, NULL, &str, SLP_TRUE);
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

	/**** Test the freshening. ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, "(abool=false),keyw,(anInt=51)");
	assert(err == SLP_OK);

	err = SLPAttrSerialize(attr, NULL, &str, SLP_TRUE);
	assert(err == SLP_OK);
	assert(strstr(str, "(abool=false)") != NULL);
	assert(strstr(str, "keyw") != NULL);
	assert(strstr(str, "(anInt=51)") != NULL);
	assert(strlen(str) == 29);
	free(str);
	
	err = SLPAttrSet_bool(attr, "abool", SLP_FALSE);
	assert(err == SLP_OK);

	err = SLPAttrSerialize(attr, NULL, &str, SLP_TRUE);
	assert(err == SLP_OK);
	assert(strcmp("(abool=false)", str) == 0);
	free(str);
	
	SLPAttrFree(attr);

	/**** Test serialization. ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, "(a=1)");
	assert(err == SLP_OK);

	err = SLPAttrSerialize(attr, NULL, &str, SLP_TRUE);
	assert(err == SLP_OK);
	assert(strcmp(str, "(a=1)") == 0);
	free(str);
	
	SLPAttrFree(attr);

	
	/**** Test serialization. ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, "(a=0)");
	assert(err == SLP_OK);

	err = SLPAttrSerialize(attr, NULL, &str, SLP_TRUE);
	assert(err == SLP_OK);
	assert(strcmp(str, "(a=0)") == 0);
	free(str);
	
	SLPAttrFree(attr);

	/**** Test serialization. ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, "(a=-1)");
	assert(err == SLP_OK);

	err = SLPAttrSerialize(attr, NULL, &str, SLP_TRUE);
	assert(err == SLP_OK);
	assert(strcmp(str, "(a=-1)") == 0);
	free(str);
	
	SLPAttrFree(attr);

	/**** Test serialization. ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, "(a=52)");
	assert(err == SLP_OK);

	err = SLPAttrSerialize(attr, NULL, &str, SLP_TRUE);
	assert(err == SLP_OK);
	assert(strcmp(str, "(a=52)") == 0);
	free(str);
	
	SLPAttrFree(attr);

	/**** Test serialization. ****/
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &attr, "(a=-2000)");
	assert(err == SLP_OK);

	err = SLPAttrSerialize(attr, NULL, &str, SLP_TRUE);
	assert(err == SLP_OK);
	assert(strcmp(str, "(a=-2000)") == 0);
	free(str);
	
	SLPAttrFree(attr);

	return 0;
}
