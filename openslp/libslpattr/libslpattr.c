
#include <stdlib.h>
#include <string.h>
//#include <slp.h>
#include <assert.h>
#include <math.h>
#include <ctype.h>

#include <libslpattr.h>

/* The strings used to represent non-string variables. */
#define BOOL_TRUE_STR "true"
#define BOOL_TRUE_STR_LEN 4
#define BOOL_FALSE_STR "false"
#define BOOL_FALSE_STR_LEN 5


/* The preamble to every variable. */
#define VAR_PREFIX "("
#define VAR_PREFIX_LEN 1

#define VAR_INFIX "="
#define VAR_INFIX_LEN 1

#define VAR_SUFFIX ")"
#define VAR_SUFFIX_LEN 1

#define VAR_SEPARATOR ","
#define VAR_SEPARATOR_LEN 1

/* The cost of the '(=)' for a non-keyword attribute. */
#define VAR_NON_KEYWORD_SEMANTIC_LEN VAR_PREFIX_LEN + VAR_INFIX_LEN + VAR_SUFFIX_LEN


/* The character with which to escape other characters. */
#define ESCAPE_CHARACTER '\\'

/* The number of characters required to escape a single character. */
#define ESCAPED_LEN 3

/* The preamble for opaques ('\FF') -- this is only used when the attributes
 * are "put on the wire". */
#define OPAQUE_PREFIX "\\FF"
#define OPAQUE_PREFIX_LEN 3



/******************************************************************************
 *
 *                                   Utility
 *
 *****************************************************************************/

/* Tests a character to see if it reserved (as defined in RFC 2608, p11). */
#define IS_RESERVED(x) \
	(((x) == '(' || (x) == ')' || (x) == ',' || (x) == '\\' || (x) == '!' || (x) == '<' \
	|| (x) == '=' || (x) == '>' || (x) == '~') || \
	((((char)0x01 <= (x)) && ((char)0x1F >= (x))) || ((x) == (char)0x7F)))
	

#define IS_INVALID_VALUE_CHAR(x) \
	IS_RESERVED(x)

#define IS_INVALID_TAG_CHAR(x) \
	(IS_RESERVED(x) \
	|| ((x) == '*') || \
	((x) == (char)0x0D) || ((x) == (char)0x0A) || ((x) == (char)0x09) || ((x) == '_'))

#define IS_VALID_TAG_CHAR(x) !IS_INVALID_TAG_CHAR(x)


/* Find the end of a tag, while checking that said tag is valid. 
 *
 * Returns: Pointer to the character immediately following the end of the tag,
 * or NULL if the tag is improperly formed. 
 */
char *find_tag_end(char *tag) {
	char *cur = tag; /* Pointer into the tag for working. */

	while (*cur) {
		if (IS_INVALID_TAG_CHAR(*cur)) {
			break;
		}
		cur++;
	}

	return cur;
}


/* Unescapes an escaped character. 
 *
 * val should point to the escape character starting the value.
 */
char unescape(char d1, char d2) {
	assert(isxdigit(d1));
	assert(isxdigit(d2));

	if ((d1 >= 'A') && (d1 <= 'F')) 
		d1 = d1 - 'A' + 0x0A;
	else 
    	d1 = d1 - '0';

    if ((d2 >= 'A') && (d2 <= 'F'))
        d2 = d2 - 'A' + 0x0A;
    else 
        d2 = d2 - '0';

    return d2 + (d1 * 0x10);
}


/* Unescapes a string. 
 *
 * Returns: Pointer to start of unescaped string. If an error occurs, NULL is
 * returned (an error consists of an escaped value being truncated).
 */
char *unescape_into(char *dest, const char *src, size_t len) {
	char *start, *write;
	size_t i;
	
	assert(dest);
	assert(src);
	
	write = start = dest;
	
	for (i = 0; i < len; i++, write++) {
		if (src[i] == ESCAPE_CHARACTER) {
			/*** Check that the characters are legal, and that the value has
			 * not been truncated. 
			 ***/
			if ((i + 2 < len) && isxdigit(src[i+1]) && isxdigit(src[i+2])) {
				*write = unescape(src[i+1], src[i+2]);
				i += 2;
			} 
			else {
				return NULL;
			}
		} 
		else {
			*write = src[i];
		}
	}

	return start;
}


/* Finds the end of a value, while checking that the value contains legal
 * characters. 
 *
 * Returns: see find_tag_end().
 */
char *find_value_end(char *value) {
	char *cur = value; /* Pointer  into the value string. */

	while (*cur) {
		if (IS_INVALID_VALUE_CHAR(*cur) && (*cur != '\\')) {
			break;
		}
		cur++;
	}

	return cur;
}


/* Find the number of digits (base 10) necessary to represent an integer. 
 *
 * Returns the number of digits.
 */
int count_digits(long number) {
	int count = 0;

	/***** As far as I recall, log is undefined at one... *****/
	if (number == 1) {
		return 1;
	}
	if (number == -1) {
		return 2;
	}
	
	/***** Does it require a negative sign? *****/
	if (number < 0) {
		count += 1;
	}

	/***** Count digits *****/
	/*** Can't take the log of zero. ***/
	if (number == 0) {
		return 1 + count;
	}

	return count + (int)(ceil(log10((double)labs(number))));
}


/* Verifies that a tag contains only legal characters. */
SLPBoolean is_valid_tag(const char *tag) {
	/* TODO Check. */
	return SLP_TRUE;
}


/* A boolean expression that tests a character to see if it must be escaped.
 */
#define ATTRIBUTE_RESERVED_TEST(x) \
	(x == '(' || x == ')' || x == ',' || x == '\\' || x == '!' || x == '<' \
	 || x == '=' || x == '<' || x == '=' || x == '>' || x == '~' || x == '\0')
/* Tests a character to see if it should be escaped. To be used for everything
 * but opaques. */
SLPBoolean is_legal_slp_char(const char to_test) {
	if (ATTRIBUTE_RESERVED_TEST(to_test)) {
		return SLP_FALSE;
	}
	return SLP_TRUE;
}


/* Tests a character to see if it should be escaped for an opaque. */
SLPBoolean opaque_test(const char to_test) {
	return SLP_FALSE;
}


/* Find the size of an unescaped string (given the escaped string). 
 *
 * Note that len must be positive. 
 *
 * Returns: If positive, the length of the string. If negative, there is some
 * sort of error. 
 */
int find_unescaped_size(const char *str, int len) {
	int i;
	int size;

	assert(len > 0);
	
	size = len;
	
	for (i = 0; i < len; i++) {
		if (str[i] == ESCAPE_CHARACTER) {
			size -= ESCAPED_LEN - 1; /* -1 for the ESCAPE_CHARACTER. */
		}
	}

	return size;
}


/* Find the size of an escaped string. 
 *
 * The "optional" len argument is the length of the string.  If it is
 * negative, the function treats the string as a null-terminated string. If it
 * is positive, the function will read exactly that number of characters. 
 */
int find_escaped_size(const char *str, int len) {
	int i; /* Index into str. */
	int escaped_size; /* The size of the thingy. */
	
	i =0;
	escaped_size = 0;
	if (len < 0) {
		/***** String is null-terminated. *****/
		for(i = 0; str[i]; i++) {
			if (is_legal_slp_char(str[i]) == SLP_TRUE) {
				escaped_size++;
			} else {
				escaped_size += ESCAPED_LEN;
			}
		}
	} else {
		for (i = 0; i < len; i++) {
			if (is_legal_slp_char(str[i]) == SLP_TRUE) {
				escaped_size++;
			} else {
				escaped_size += ESCAPED_LEN;
			}
		}
	}

	return escaped_size;
}


/* Escape a single character. Writes the escaped value into dest, and
 * increments dest. 
 *
 * NOTE: Most of this code is stolen from Dave McCormack's SLPEscape() code.
 * (For OpenSLP). 
 */
void escape(char to_escape, char **dest, SLPBoolean (test_fn)(const char)) {
	char hex_digit;
	if (test_fn(to_escape) == SLP_FALSE)
	{
        /* Insert the escape character. */
        **dest = ESCAPE_CHARACTER;
        (*dest)++;

        /* Do the first digit. */
        hex_digit = (to_escape & 0xF0)/0x0F;
        if  ((hex_digit >= 0) && (hex_digit <= 9))
            **dest = hex_digit + '0';
        else
            **dest = hex_digit + 'A' - 0x0A;

        (*dest)++;

        /* Do the last digit. */
        hex_digit = to_escape & 0x0F;
        if ((hex_digit >= 0) && (hex_digit <= 9))
			**dest = hex_digit + '0';
        else
			**dest = hex_digit + 'A' - 0x0A;
		(*dest)++;
	}
	else
	{
		/* It's legal. */
        **dest = to_escape;
		(*dest)++;
	} 
}


/* Escape the passed string (src), writing it into the other passed string
 * (dest). 
 *
 * If the len argument is negative, the src is treated as null-terminated,
 * otherwise that length is escaped. 
 *
 * Returns a pointer to where the addition has ended. 
 */
char *escape_into(char *dest, char *src, int len) {
	char *cur_dest; /* Current character in dest. */
	cur_dest = dest;
	if (len < 0) {
		/* Treat as null terminated. */
		char *cur_src; /* Current character in src. */

		cur_src = src;
		for (; *cur_src; cur_src++) {
			escape(*cur_src, &cur_dest, is_legal_slp_char);
		}
	} else {
		/* known length. */
		int i; /* Index into src. */
		for (i = 0; i < len; i++) {
			escape(src[i], &cur_dest, is_legal_slp_char);
		}
	}
	return cur_dest;
}


/* Special case for escaping opaque strings. Escapes _every_ character in the
 * string. 
 *
 * Note that the size parameter _must_ be defined. 
 * 
 * Returns a pointer to where the addition has ended. 
 */
char *escape_opaque_into(char *dest, char *src, size_t len) {
	int i; /* Index into src. */
	char *cur_dest;

	cur_dest = dest; 

	for (i = 0; i < len; i++) {
		escape(src[i], &cur_dest, opaque_test);
	}

	return cur_dest;

}


/******************************************************************************
 *
 *                              Individual values
 *
 * What is a value?
 *
 * In SLP an individual attribute can be associated with a list of values. A
 * value is the data associated with a tag. Depending on the type of
 * attribute, there can be zero, one, or many values associated with a single
 * tag. 
 *****************************************************************************/

typedef struct xx_value_t {
	struct xx_value_t *next;
	int escaped_len;
	int unescaped_len;
	union {
		SLPBoolean va_bool;
		long va_int;
		char *va_str; /* This is used for keyword, string, and opaque. */
	} data; /* Stores the value of the variable. Note, any string must be copied into the struct. */
} value_t;


/* Create and initialize a new value. 
 */
value_t *value_new() {
	value_t *value = NULL;

	value = (value_t *)malloc(sizeof(value_t));
	if (value == NULL) 
		return NULL;
	value->next = NULL;
	value->data.va_str = NULL;

	value->escaped_len = -1;
	value->unescaped_len = -1;
	
	return value;
}


/* Destroy a value. */
void value_free(value_t *value) {
	assert(value->next == NULL);
	free(value);
}

/******************************************************************************
 *
 *                              Individual attributes (vars)
 *
 *  What is a var? 
 *
 *  A var is a variable tag that is associated with a list of values. Zero or
 *  more vars are kept in a single SLPAttributes object. Each value stored in
 *  a var is kept in a value struct. 
 *****************************************************************************/

/* An individual attribute in the struct. */
typedef struct xx_var_t {
	struct xx_var_t *next; /* Pointer to the next variable. */
	SLPType type; /* The type of this variable. */
	const char *tag; /* The name of this variable. */
	unsigned int tag_len; /* The length of the tag. */
	value_t *list; /* The list of values. */
	int list_size; /* The number of values in the list. */
	SLPBoolean modified; /* Flag. Set to be true if the attribute should be included in the next freshen.  */
} var_t;


/* Create a new variable. */
var_t *var_new(char *tag) {
	var_t *var; /* Variable being created. */
	
	var = (var_t *)malloc(sizeof(var_t));
	
	assert(tag != NULL);
	
	if (var == NULL)
		return NULL;
	
	var->next = NULL;

	var->tag_len = strlen(tag);
	
	var->tag = (char *)malloc( var->tag_len + 1);
	if (var->tag == NULL) {
		free(var);
		return NULL;
	}
	strcpy((char *)var->tag, tag);
	
	var->type = -1;

	var->list = NULL;
	var->list_size = 0;
	
	var->modified = SLP_TRUE;

	return var;
}


/* Destroy a value list. Note that the var is not free()'d, only reset. */
void var_list_destroy(var_t *var) {
	value_t *value;
	
	if (var->list == NULL) {
		assert(var->list_size == 0);
		return;
	}
	
	var->list_size = 0;
	switch (var->type) {
		case(SLP_KEYWORD):
			/*** Ensure that there is _no_ value. ***/
			assert(var->list == NULL);
			break;
		case(SLP_BOOLEAN):
			/*** Ensure that the boolean has only one value. ***/
			assert(var->list->next == NULL);
		case(SLP_INTEGER):
			/*** Free values ***/
			value = var->list;
			while (value) {
				value_t *next;
				next = value->next;
				value->data.va_str = NULL;
				value->next = NULL;
				
				value_free(value);

				value = next;
			}
			break;
		case(SLP_STRING):
		case(SLP_OPAQUE):
			/*** Free values ***/
			value = var->list;
			while (value) {
				value_t *next;
				next = value->next;
				if (value->data.va_str != NULL) {
					free(value->data.va_str);
					value->data.va_str = NULL;
				}
				value->next = NULL;
				
				value_free(value);
				value = next;
			}
			break;
		default:
			assert(0);
	};
	var->list = NULL;
}


/* Frees a variable. */
void var_free(var_t *var) {
	/***** Sanity check. *****/
	assert(var->next == NULL);

	free((void *)var->tag);
	
	/***** Free variable. *****/
	var_list_destroy(var);

	free(var);
}


/* Adds a value to a variable. */
SLPError var_insert(var_t *var, value_t *value, SLPInsertionPolicy policy) {
	assert(policy == SLP_ADD || policy == SLP_REPLACE);
		
	if (value == NULL) {
		return SLP_OK;
	}
	
	if (policy == SLP_REPLACE) {
		var_list_destroy(var);
	}

	/* Update list. */
	value->next = var->list;
	var->list = value;
	var->list_size++;

	/* Set mod flag.*/
	var->modified = SLP_TRUE;
	
	return SLP_OK;
}


/******************************************************************************
 *
 *                             All the attributes. 
 *
 *****************************************************************************/

/* The opaque struct representing a SLPAttributes handle.
 */
struct xx_SLPAttributes {
	SLPBoolean strict; /* Are we using strict typing? */
	char *lang; /* Language. */
	var_t *attrs; /* List of vars to be sent. */
	size_t attr_count; /* The number of attributes */
};


/*
 * SLPAttrAlloc() creates and initializes a new instance of SLPAttributes. 
 */
SLPError SLPAttrAlloc(
		const char *lang, 
		const FILE *template_h,
		const SLPBoolean strict, 
		SLPAttributes *slp_attr_h
) {
	struct xx_SLPAttributes **slp_attr;
	slp_attr = (struct xx_SLPAttributes **)slp_attr_h;

	/***** Sanity check *****/
	if (strict == SLP_FALSE && template_h != NULL) {
		/* We can't be strict if we don't have a template. */
		return SLP_PARAMETER_BAD;
	}

	if (strict != SLP_FALSE) {
		return SLP_NOT_IMPLEMENTED;
	}
	if (template_h != NULL) {
		return SLP_NOT_IMPLEMENTED;
	}
	
	/***** Create. *****/
	(*slp_attr) = (struct xx_SLPAttributes *)malloc( sizeof(struct xx_SLPAttributes) );
	
	if (*slp_attr == NULL) {
		return SLP_MEMORY_ALLOC_FAILED;
	}

	/***** Initialize *****/
	(*slp_attr)->strict = SLP_FALSE; /* FIXME Add templates. */
	(*slp_attr)->lang = strdup(lang); /* free()'d in SLPAttrFree(). */
	(*slp_attr)->attrs = NULL;
	(*slp_attr)->attr_count = 0;

	/***** Report. *****/
	return SLP_OK;
}


SLPError attr_destringify(struct xx_SLPAttributes *slp_attr, char *str, SLPInsertionPolicy); 

/* Allocates a new thingy from a string. */
SLPError SLPAttrAllocStr(
		const char *lang, 
		const FILE *template_h,
		const SLPBoolean strict, 
		SLPAttributes *slp_attr_h,
		const char *str
) {
	SLPError err;
	char *mangle; /* A copy of the passed in string, since attr_destringify tends to chew data. */
	
	err = SLPAttrAlloc(lang, template_h, strict, slp_attr_h);

	if (err != SLP_OK) {
		return err;
	}
	
	mangle = strdup(str);
	if (str == NULL) {
		SLPAttrFree(*slp_attr_h);
		return SLP_MEMORY_ALLOC_FAILED;
	}
	err = attr_destringify((struct xx_SLPAttributes*)*slp_attr_h, mangle, SLP_ADD);
	free(mangle);

	if (err != SLP_OK) {
		SLPAttrFree(*slp_attr_h);
	}

	return err;
}


/* Destroys the passed SLPAttributes(). 
 */
void SLPAttrFree(SLPAttributes slp_attr_h) {
	struct xx_SLPAttributes *slp_attr;
	slp_attr = (struct xx_SLPAttributes *)slp_attr_h;
	
	/***** Free held resources. *****/
	while (slp_attr->attrs) {
		var_t *attr = slp_attr->attrs;
		slp_attr->attrs = attr->next;

		attr->next = NULL;

		var_free(attr);
	}
	
	free(slp_attr->lang);
	slp_attr->lang = NULL;
	
	/***** Free the handle *****/
	free(slp_attr);
	
	slp_attr = NULL;

}


/* Insert a variable into the var list. */
void attr_add(struct xx_SLPAttributes *slp_attr, var_t *var) {
	var->next = slp_attr->attrs;
	slp_attr->attrs = var; 

	slp_attr->attr_count++;
}


/* Find a variable by its tag.
 *
 * Returns a NULL if the value could not be found.
 */
var_t *attr_val_find_str(struct xx_SLPAttributes *slp_attr, const char *tag) {
	var_t *var;

	var = slp_attr->attrs;
	while (var) {
		if (strcmp(var->tag, tag) == 0) {
			return var;
		}
		var = var->next;
	}

	return NULL;
}

/* Test a variable's type. Returns SLP_OK if the match is alright, or some
 * other error code (meant to be forwarded to the application) if the match is
 * bad.
 */
SLPError attr_type_verify(struct xx_SLPAttributes *slp_attr, var_t *var, SLPType type) {
	assert(var->type != -1); /* Check that it's been set. */
	if (var->type == type) {
		return SLP_OK;
	}

	return SLP_TYPE_ERROR; /* FIXME: Check against template. */
}

/******************************************************************************
 *
 *                              Setting attributes
 *
 *****************************************************************************/

/*****************************************************************************/
SLPError generic_set_val(struct xx_SLPAttributes *slp_attr, const char *tag, value_t *value, SLPInsertionPolicy policy, SLPType attr_type) 
/* Finds and sets the value named in tag.                                    */
/* 
 * slp_attr  - The attr object to add to.
 * tag       - the name of the tag to add to.
 * value     - the already-allocated value object with fields set
 * policy    - the policy to use when inserting.
 * attr_type - the type of the value being added.
 *****************************************************************************/
{
	var_t *var;
	/***** Create a new attribute. *****/	
	if ( (var = attr_val_find_str(slp_attr, tag)) == NULL) {	
		/*** Couldn't find a value with this tag. Make a new one. ***/	
		var = var_new((char *)tag);	
		if (var == NULL) {	
			return SLP_MEMORY_ALLOC_FAILED;	
		}	
		var->type = attr_type;	
		/** Add variable to list. **/	
		attr_add(slp_attr, var);	
	} else {
		SLPError err;
		/*** The attribute already exists. ***/ 
		/*** Verify type. ***/	
		err = attr_type_verify(slp_attr, var, attr_type);	
		if (err == SLP_TYPE_ERROR && policy == SLP_REPLACE) { 
			var_list_destroy(var); 
			var->type = attr_type; 
		} 
		else if (err != SLP_OK) {	
			value_free(value); 
			return err;	
		}	
	}	
	/***** Set value *****/	
	var_insert(var, value, policy); 

	return SLP_OK;
}


/* Set a boolean attribute.  */
SLPError SLPAttrSet_bool(
		SLPAttributes attr_h,
		const char *tag,
		SLPBoolean val
) {
	struct xx_SLPAttributes *slp_attr = (struct xx_SLPAttributes *)attr_h;
	value_t *value = NULL;
	
	/***** Sanity check. *****/
	if (val != SLP_TRUE && val != SLP_FALSE) {
		return SLP_PARAMETER_BAD;
	}
	if ( is_valid_tag(tag) == SLP_FALSE ) {
		return SLP_TAG_BAD;
	}

	/***** Set the initial (and only) value. *****/
	/**** Create ****/
	value = value_new();

	assert(value);

	/*** Set escaped information. ***/
	if (val == SLP_TRUE) {
		value->escaped_len = BOOL_TRUE_STR_LEN;
	} 
	else {
		value->escaped_len = BOOL_FALSE_STR_LEN;
	}
	value->data.va_bool = val;

	/**** Set the value and return. ****/
	return generic_set_val(slp_attr, tag, value, SLP_REPLACE, SLP_BOOLEAN);
}


/* Sets a string attribute. */
SLPError SLPAttrSet_str(
		SLPAttributes attr_h,
		const char *tag,
		const char *val,
		SLPInsertionPolicy policy
) {
	struct xx_SLPAttributes *slp_attr = (struct xx_SLPAttributes *)attr_h;
	value_t *value;
	
	/***** Sanity check. *****/
	if ( is_valid_tag(tag) == SLP_FALSE ) {
		return SLP_TAG_BAD;
	}
	if ( val == NULL ) {
		return SLP_PARAMETER_BAD;
	}

	/***** Create new value. *****/
	value = value_new();
	assert(value);
	
	value->data.va_str = strdup(val);
	if (value->data.va_str == NULL) {
		value_free(value);
		return SLP_MEMORY_ALLOC_FAILED;
	}

	value->escaped_len = find_escaped_size(value->data.va_str, -1);
	
	return generic_set_val(slp_attr, tag, value, policy, SLP_STRING);
	return SLP_OK;
}


/* Sets a keyword attribute. */
SLPError SLPAttrSet_keyw(
		SLPAttributes attr_h,
		const char *tag
) {
	struct xx_SLPAttributes *slp_attr = (struct xx_SLPAttributes *)attr_h;
	
	/***** Sanity check. *****/
	if ( is_valid_tag(tag) == SLP_FALSE ) {
		return SLP_TAG_BAD;
	}

	return generic_set_val(slp_attr, tag, NULL, SLP_REPLACE, SLP_KEYWORD);
}


/* Sets an integer attribute. */
SLPError SLPAttrSet_int(
		SLPAttributes attr_h,
		const char *tag,
		long val,
		SLPInsertionPolicy policy
) {
	struct xx_SLPAttributes *slp_attr = (struct xx_SLPAttributes *)attr_h;
	value_t *value;
	
	/***** Sanity check. *****/
	if ( is_valid_tag(tag) == SLP_FALSE ) {
		return SLP_TAG_BAD;
	}

	/***** Create new value. *****/
	value = value_new();
	if (value == NULL) {
		return SLP_MEMORY_ALLOC_FAILED;
	}
	
	/**** Set ****/
	value->data.va_int = val;
	value->escaped_len = count_digits(value->data.va_int);
	assert(value->escaped_len > 0);

	return generic_set_val(slp_attr, tag, value, policy, SLP_INTEGER);
}


/* Set an opaque attribute. */
SLPError SLPAttrSet_opaque(
		SLPAttributes attr_h,
		const char *tag,
		const char *val,
		const unsigned int len, 
		SLPInsertionPolicy policy
) {
	struct xx_SLPAttributes *slp_attr = (struct xx_SLPAttributes *)attr_h;
	value_t *value;
	
	/***** Sanity check. *****/
	if ( is_valid_tag(tag) == SLP_FALSE ) {
		return SLP_TAG_BAD;
	}
	if ( val == NULL ) {
		return SLP_PARAMETER_BAD;
	}

	/***** Create a new attribute. *****/
	value = value_new();
	if (value == NULL) {
		return SLP_MEMORY_ALLOC_FAILED;
	}
	
	value->data.va_str = (char *)malloc( len );
	
	if (value->data.va_str == NULL) {
		free(value->data.va_str);
		return SLP_MEMORY_ALLOC_FAILED;
	}
	
	memcpy((void *)value->data.va_str, val, len);
	value->unescaped_len = len;
	value->escaped_len = (len * ESCAPED_LEN) + OPAQUE_PREFIX_LEN;

	return generic_set_val(slp_attr, tag, value, policy, SLP_OPAQUE);
}


SLPError SLPAttrStore(struct xx_SLPAttributes *slp_attr, 
		const char *tag,
		const char *val,
		size_t len,
		SLPInsertionPolicy policy
);

/* Set an attribute of unknown type. 
 *
 * Note that the policy in this case is a special case: If the policy is 
 * SLP_REPLACE, we delete the current list and replace it with the passed 
 * value. If it's a multivalue list, we replace the current value with the 
 * ENTIRE passed list. 
 *
 * FIXME Should we "elide" whitespace?
 */
SLPError SLPAttrSet_guess(
		SLPAttributes attr_h,
		const char *tag,
		const char *val,
		SLPInsertionPolicy policy
) {
	SLPError err;
	size_t len;
	const char *cur, *end;
	
	/***** Sanity check. *****/
	if ( is_valid_tag(tag) == SLP_FALSE ) {
		return SLP_TAG_BAD;
	}
	if ( val == NULL ) {
		return SLP_PARAMETER_BAD;
	}

	/***** 
	 * If we have a replace policy and we're inserting a multivalued list, 
	 * the values will clobber each other. Therefore if we have a replace, we 
	 * delete the current list, and use an add policy.
	 *****/
	if (policy == SLP_REPLACE) {
		var_t *var;
		var = attr_val_find_str((struct xx_SLPAttributes *)attr_h, tag);
		if (var) {
			var_list_destroy(var);
		}
	}
	
	/***** Check for multivalue list. *****/
	cur = val;
	do {
		end = strstr(cur, VAR_SEPARATOR);
		if (end == NULL) {
			len = strlen(cur);
		} else {
			/*** It's multivalue. ***/
			len = end - cur;
		}

		err = SLPAttrStore((struct xx_SLPAttributes *)attr_h, tag, cur, len, SLP_ADD);
		if (err != SLP_OK) {
			/* FIXME Ummm. Should we return or ignore? */
			return err;
		}
		cur = end + VAR_SEPARATOR_LEN;
	} while(end);

	/***** Return *****/
	return SLP_OK;
}



/******************************************************************************
 *
 *                              Getting attributes
 *
 *****************************************************************************/

/* Get the value of a boolean attribute. Note that it _cannot_ be multivalued. 
 */
SLPError SLPAttrGet_bool(
		SLPAttributes attr_h,
		const char *tag,
		SLPBoolean *val
) {
	struct xx_SLPAttributes *slp_attr = (struct xx_SLPAttributes *)attr_h;
	var_t *var;

	var = attr_val_find_str(slp_attr, tag);

	/***** Check that the tag exists. *****/
	if (var == NULL) {
		return SLP_TAG_ERROR;
	}

	/* TODO Verify type against template. */
	
	/***** Verify type. *****/
	if (var->type != SLP_BOOLEAN) {
		return SLP_TYPE_ERROR;
	}

	assert(var->list != NULL);
	
	*val = var->list->data.va_bool;
	
	return SLP_OK;
}

/* Get the value of a keyword attribute. Since keywords either exist or don't
 * exist, no value is passed out. Instead, if the keyword exists, an SLP_OK is
 * returned, if it doesn't exist, an SLP_TAG_ERROR is returned. If the tag
 * does exist, but is associated with a non-keyword attribute, SLP_TYPE_ERROR
 * is returned. 
 */
SLPError SLPAttrGet_keyw(
		SLPAttributes attr_h,
		const char *tag
) {
	struct xx_SLPAttributes *slp_attr = (struct xx_SLPAttributes *)attr_h;
	var_t *var;

	var = attr_val_find_str(slp_attr, tag);

	/***** Check that the tag exists. *****/
	if (var == NULL) {
		return SLP_TAG_ERROR;
	}

	/* TODO Verify type against template. */
	
	/***** Verify type. *****/
	if (var->type != SLP_KEYWORD) {
		return SLP_TYPE_ERROR;
	}

	assert(var->list == NULL);
	
	return SLP_OK;
}


/* Get an integer value. Since integer attributes can be multivalued, an array
 * is returned that contains all values corresponding to the given tag. 
 *
 *
 * Note: On success, an array of SLP_INTEGERs is created. It is the caller's
 * responsibility to free the memory returned through val. 
 * 
 * Returns:
 *  SLP_OK
 *    Returned if the attribute is found. The array containing the values is
 *    placed in val, and size is set to the number of values in val. 
 *  SLP_TYPE_ERROR
 *    Returned if the tag exists, but the associated value is not of type
 *    SLP_INTEGER.
 *  SLP_MEMORY_ALLOC_FAILED
 *    Memory allocation failed. 
 */
SLPError SLPAttrGet_int(
		SLPAttributes attr_h,
		const char *tag,
		long **val,
		size_t *size
) {
	struct xx_SLPAttributes *slp_attr = (struct xx_SLPAttributes *)attr_h;
	var_t *var;
	value_t *value;
	size_t i;

	var = attr_val_find_str(slp_attr, tag);

	/***** Check that the tag exists. *****/
	if (var == NULL) {
		return SLP_TAG_ERROR;
	}

	/* TODO Verify type against template. */
	
	/***** Verify type. *****/
	if (var->type != SLP_INTEGER) {
		return SLP_TYPE_ERROR;
	}

	/***** Create return value. *****/
	*size = var->list_size;
	*val = (long *)malloc( sizeof(long) * var->list_size );
	if (*val == NULL) {
		return SLP_MEMORY_ALLOC_FAILED;
	}

	/***** Set values *****/
	assert(var->list != NULL);
	value = var->list;
	for (i = 0; i < var->list_size; i++, value = value->next) {
		assert(value != NULL);
		(*val)[i] = value->data.va_int;
	}

	return SLP_OK;
}

/* Get string values. Since string attributes can be multivalued, an array
 * is returned that contains all values corresponding to the given tag. 
 *
 *
 * Note: On success, an array of SLP_STRINGs is created. It is the caller's
 * responsibility to free the memory returned through val. Note that the array
 * referencing the strings is allocated separately from each string value,
 * meaning that each value must explicitly be deallocated. 
 * 
 * Returns:
 *  SLP_OK
 *    Returned if the attribute is found. The array containing the values is
 *    placed in val, and size is set to the number of values in val. 
 *  SLP_TYPE_ERROR
 *    Returned if the tag exists, but the associated value is not of type
 *    SLP_INTEGER.
 *  SLP_MEMORY_ALLOC_FAILED
 *    Memory allocation failed. 
 */
SLPError SLPAttrGet_str(
		SLPAttributes attr_h,
		const char *tag,
		char ***val,
		size_t *size
) {
	struct xx_SLPAttributes *slp_attr = (struct xx_SLPAttributes *)attr_h;
	var_t *var;
	value_t *value;
	size_t i;

	var = attr_val_find_str(slp_attr, tag);

	/***** Check that the tag exists. *****/
	if (var == NULL) {
		return SLP_TAG_ERROR;
	}

	/* TODO Verify type against template. */
	
	/***** Verify type. *****/
	if (var->type != SLP_STRING) {
		return SLP_TYPE_ERROR;
	}

	/***** Create return value. *****/
	*size = var->list_size;
	*val = (char **)malloc( sizeof(char *) * var->list_size );
	if (*val == NULL) {
		return SLP_MEMORY_ALLOC_FAILED;
	}

	/***** Set values *****/
	assert(var->list != NULL);
	value = var->list;
	for (i = 0; i < var->list_size; i++, value = value->next) {
		assert(value != NULL);
		(*val)[i] = strdup(value->data.va_str);
	}

	return SLP_OK;
}

/* Get opaque values. Since opaque attributes can be multivalued, an array
 * is returned that contains all values corresponding to the given tag. 
 *
 *
 * Note: On success, an array of SLP_OPAQUEs is created. It is the caller's
 * responsibility to free the memory returned through val. Note that the array
 * referencing the opaques is allocated separately from each opaque struct,
 * and from the corresponding opaque value, meaning that each value must 
 * explicitly be deallocated, as must each opaque struct. 
 * 
 * Returns:
 *  SLP_OK
 *    Returned if the attribute is found. The array containing the values is
 *    placed in val, and size is set to the number of values in val. 
 *  SLP_TYPE_ERROR
 *    Returned if the tag exists, but the associated value is not of type
 *    SLP_INTEGER.
 *  SLP_MEMORY_ALLOC_FAILED
 *    Memory allocation failed. 
 */
SLPError SLPAttrGet_opaque(
		SLPAttributes attr_h,
		const char *tag,
		SLPOpaque ***val,
		size_t *size
) {
	struct xx_SLPAttributes *slp_attr = (struct xx_SLPAttributes *)attr_h;
	var_t *var;
	value_t *value;
	size_t i;

	var = attr_val_find_str(slp_attr, tag);

	/***** Check that the tag exists. *****/
	if (var == NULL) {
		return SLP_TAG_ERROR;
	}

	/* TODO Verify type against template. */
	
	/***** Verify type. *****/
	if (var->type != SLP_OPAQUE) {
		return SLP_TYPE_ERROR;
	}

	/***** Create return value. *****/
	*size = var->list_size;
	*val = (SLPOpaque **)malloc( sizeof(SLPOpaque *) * var->list_size );
	if (*val == NULL) {
		return SLP_MEMORY_ALLOC_FAILED;
	}

	/***** Set values *****/
	assert(var->list != NULL);
	value = var->list;
	for (i = 0; i < var->list_size; i++, value = value->next) {
		assert(value != NULL);
		(*val)[i] = (SLPOpaque *)malloc( sizeof(SLPOpaque) );
		if ( (*val)[i]->data == NULL ) {
			/* TODO Deallocate everything and return. */
			return SLP_MEMORY_ALLOC_FAILED;
		}
		(*val)[i]->len = value->unescaped_len;
		(*val)[i]->data = (char *)malloc( value->unescaped_len );
		if ( (*val)[i]->data == NULL ) {
			/* TODO Deallocate everything and return. */
			return SLP_MEMORY_ALLOC_FAILED;
		}
		memcpy((*val)[i]->data, value->data.va_str, value->unescaped_len);
	}

	return SLP_OK;
}

/* Finds the type of the given attribute. 
 *
 * Returns:
 *  SLP_OK
 *    If the attribute is set. The type is returned through the type
 *    parameter.
 *  SLP_TAG_ERROR
 *    If the attribute is not set. 
 */
SLPError SLPAttrGetType(SLPAttributes attr_h, const char *tag, SLPType *type) {
	struct xx_SLPAttributes *slp_attr = (struct xx_SLPAttributes *)attr_h;
	var_t *var;

	var = attr_val_find_str(slp_attr, tag);

	/***** Check that the tag exists. *****/
	if (var == NULL) {
		return SLP_TAG_ERROR;
	}
	
	if (type != NULL) {
		*type = var->type;
	}
	
	return SLP_OK;
}



/******************************************************************************
 *
 *                          Attribute (En|De)coding 
 *
 *****************************************************************************/

/* Gets the escaped stringified version of an attribute list. 
 *
 * The string returned must be free()'d by the caller.
 *
 * Params:
 * attr_h -- (IN) Attribute handle to add serialize.
 * tags -- (IN) The tags to serialize. If NULL, all tags are serialized. 
 * out_buffer -- (IN/OUT) A buffer to write the serialized string to. If 
 *               (*out_buffer == NULL), then a new buffer is allocated by 
 *               the API. 
 * bufferlen -- (IN) The length of the buffer. Ignored if 
 *              (*out_buffer == NULL). 
 * count -- (OUT) The size needed/used of out_buffer (includes trailing null).
 * find_delta -- (IN)  If find_delta is set to true, only the attributes that 
 *               have changed since the last serialize are updated.
 * 
 * Returns:
 * SLP_OK -- Serialization occured. 
 * SLP_BUFFER_OVERFLOW -- If (*out_buffer) is defined, but bufferlen is 
 *                    smaller than the amount of memory necessary to serialize 
 *                    the attr. list.
 * SLP_MEMORY_ALLOC_FAILED -- Ran out of memory. 
 */
SLPError SLPAttrSerialize(SLPAttributes attr_h,
		const char* tags /* NULL terminated */,
		char **out_buffer /* Where to write. if *out_buffer == NULL, space is alloc'd */,
		size_t bufferlen, /* Size of buffer. */
		size_t* count, /* Bytes needed/written. */
		SLPBoolean find_delta
) {
	struct xx_SLPAttributes *slp_attr = (struct xx_SLPAttributes *)attr_h;
	var_t *var; /* For iterating over attributes to serialize. */
	var_t *to_serialize; /* The list of elements to serialize. */
	unsigned int size; /* Size of the string to allocate. */
	unsigned int var_count; /* To count the number of variables. */
	char *build_str; /* The string that is being built. */
	char *cur; /* Current location within the already allocated str. */
	
	size = 0;
	var_count = 0;

	/***** Build a list of attributes to serialize. *****/
	if (tags == NULL || *tags == 0) 
	{
		/**** Serialize the entire attribute list. ****/
		to_serialize = slp_attr->attrs;
	}
	else 
	{
		/**** Create a useful tag list. ****/
		char *end;
		char *tag_string; /* A modifiable version of the tag list. */
		char *next_tag; /* A pointer to the next tag to look at. */
		size_t tag_len; /* The number of tags in tag_list. */
		size_t i; /* Index for looping overe tags. */
		int done; /* A flag for looping over tags. */

		tag_string = strdup(tags); /* free()'d at block end. */
		if (tag_string == NULL) 
		{
			return SLP_MEMORY_ALLOC_FAILED;
		}
		
		
		
		/**** Serialize a subset of the attribute list. ****/

		/*** Find the size of the tag list. ***/
		tag_len = 0;
		next_tag = tag_string;
		while ((next_tag = strchr(next_tag, ','))) 
		{
			tag_len++;
			next_tag++; /* Move past old comma. */
		}
		tag_len++; /* For the final tag. */
		
		/*** Create a copy of the tag list. ***/
		/** Malloc. **/
		to_serialize = (var_t *)malloc(tag_len * sizeof(var_t));
		if (to_serialize == NULL) 
		{
			return SLP_MEMORY_ALLOC_FAILED;
		}
		
		/** Copy. **/
		end = next_tag = tag_string;
		done = 0;
		i = 0;
		while(!done) 
		{
			var_t *to_copy;
			end = strchr(next_tag, ',');
			if (end != NULL) 
			{
				*end = 0; /* Null terminate. */
			}
			else 
			{
				done = 1;
			}
			
			to_copy = attr_val_find_str(slp_attr, next_tag);
			if (to_copy == NULL) {
				/* FIXME Cleanup. */
				free(to_serialize);
				free(tag_string);
				return SLP_TAG_ERROR;
			}
			assert(i < tag_len);
			memcpy(&to_serialize[i], to_copy, sizeof(var_t) );
			to_serialize[i].next = to_serialize + i + 1; /* Note that the last element is repointed outside this loop. */

			next_tag = end+1;
			i++;
		}
		/** Null terminate. **/
		to_serialize[tag_len-1].next = NULL;

		free(tag_string);
	}
	
	/***** Find the size of string needed for the attribute string. *****/
	for (var = to_serialize; var != NULL; var = var->next) {
		/*** Skip old attributes. ***/
		if (find_delta == SLP_TRUE && var->modified == SLP_FALSE) {
			continue;
		}
		
		/*** Get size of tag ***/
		size += var->tag_len;
		
		/*** Count the size needed for the value list ***/
		if (var->type != SLP_KEYWORD) {
			value_t *value;
		
			/** Get size of data **/
			value = var->list;
			while (value) {
				size += value->escaped_len;
				value = value->next;
			}
		
			/** Count number of commas needed for multivalued attributes. **/
			assert(var->list_size >= 0);
			size += (var->list_size - 1) * VAR_SEPARATOR_LEN;

			/** Count the semantics needed to store a multivalue list **/
			size += VAR_NON_KEYWORD_SEMANTIC_LEN;
		} else {
			assert(var->list == NULL);
		}
		
		/*** Count number of variables ***/
		var_count++;
	}

	/*** Count the number of characters between attributes. ***/
	if (var_count > 0) 
	{
		size += (var_count - 1) * VAR_SEPARATOR_LEN;
	}


	/***** Return the size needed/used. *****/
	if (count != NULL) {
		*count = size + 1;
	}
	
	/***** Create the string. *****/
	if (*out_buffer == NULL) 
	{
		/* We have to locally alloc the string. */
		build_str = (char *)malloc( size + 1);
		if (build_str == NULL) {
			/* clean up to_serialize. */
			if (to_serialize != slp_attr->attrs) {
				free(to_serialize);
			}
			return SLP_MEMORY_ALLOC_FAILED;
		}
	}
	else 
	{
		/* We write into a pre-alloc'd buffer. */
		/**** Check that out_buffer is big enough. ****/
		if (size + 1 > bufferlen) 
		{
			/* clean up to_serialize. */
			if (to_serialize != slp_attr->attrs) {
				free(to_serialize);
			}
			return SLP_BUFFER_OVERFLOW;
		}
			build_str = *out_buffer;
	}
	build_str[0] = '\0';
	
	/***** Add values *****/
	cur = build_str;
	for (var = to_serialize; var != NULL; var = var->next) { 
		/*** Skip old attributes. ***/
		if (find_delta == SLP_TRUE && var->modified == SLP_FALSE) {
			continue;
		}

		if (var->type == SLP_KEYWORD) {
			/**** Handle keywords. ****/
			strcpy(cur, var->tag);
			cur += var->tag_len;
		} else {
			/**** Handle everything else. ****/
			char *to_add;
			size_t to_add_len;
			value_t *value;
		
			/*** Add the prefix. ***/
			strcpy(cur, VAR_PREFIX);
			cur += VAR_PREFIX_LEN;
		
			/*** Add the tag. ***/
			strcpy(cur, var->tag);
			cur += var->tag_len;
		
			/*** Add the infix. ***/
			strcpy(cur, VAR_INFIX);
			cur += VAR_INFIX_LEN;
			
			/*** Insert value (list) ***/
			value = var->list;
			assert(value);
			while (value) { /* foreach member value of an attribute. */
				assert(var->type != SLP_KEYWORD);
				switch(var->type) {
					case(SLP_BOOLEAN):
						assert(value->next == NULL); /* Can't be a multivalued list. */
						assert(value->data.va_bool == SLP_TRUE 
								|| value->data.va_bool == SLP_FALSE);

						if (value->data.va_bool == SLP_TRUE) {
							to_add = BOOL_TRUE_STR;
							to_add_len = BOOL_TRUE_STR_LEN;
						} else {
							to_add = BOOL_FALSE_STR;
							to_add_len = BOOL_FALSE_STR_LEN;
						}
						strcpy(cur, to_add);
						cur += to_add_len;

						break;
					case(SLP_STRING):
						cur = escape_into(cur, value->data.va_str, -1);
						break;
					case(SLP_INTEGER):
						sprintf(cur, "%ld", value->data.va_int);
						cur += value->escaped_len;
						break;
					case(SLP_OPAQUE):
						strcpy(cur, OPAQUE_PREFIX);
						cur += OPAQUE_PREFIX_LEN;
						cur = escape_opaque_into(cur, value->data.va_str, value->unescaped_len);
						break;
					default:
						printf("Unknown type (%s:%d).\n", __FILE__, __LINE__);
						/* TODO Clean up memory leak: the output string */
						if (to_serialize != slp_attr->attrs) {
							free(to_serialize);
						}
						return SLP_INTERNAL_SYSTEM_ERROR;
				}
				
				value = value->next;
				/*** Add separator (if necessary) ***/
				if (value != NULL) {
					strcpy(cur, VAR_SEPARATOR);
					cur += VAR_SEPARATOR_LEN;
				}	
			}

			/*** Add the suffix. ***/
			strcpy(cur, VAR_SUFFIX);
			cur += VAR_SUFFIX_LEN;
		}

		/*** Add separator (if necessary) ***/
		if (var->next != NULL) {
			strcpy(cur, VAR_SEPARATOR);
			cur += VAR_SEPARATOR_LEN;
		}
			
		/*** Reset the modified flag. ***/
		var->modified = SLP_FALSE;
	}

	assert((cur - build_str) == size && size == strlen(build_str));

	/***** Append a null (This is actually done by strcpy, but its better to
	 * be safe than sorry =) *****/
	*cur = '\0';
	
	*out_buffer = build_str;
	
	/***** Free the attribute list. *****/
	if (to_serialize != slp_attr->attrs) {
		free(to_serialize);
	}
	 
	return SLP_OK;
}	


/* Stores an escaped value into an attribute. Determines type of attribute at 
 * the same time.
 *
 * tag must be null terminated. 
 * val must be of length len.
 * policy will only be respected where it can be (ints, strings, and opaques).
 *
 * the contents of tag are NOT verified. 
 * 
 * Returns: 
 *  SLP_PARAMETER_BAD - Syntax error in the value.
 *  SLP_MEMORY_ALLOC_FAILED
 */
SLPError SLPAttrStore(struct xx_SLPAttributes *slp_attr, 
		const char *tag,
		const char *val,
		size_t len,
		SLPInsertionPolicy policy
) {
	int i; /* Index into val. */
	SLPBoolean is_str; /* Flag used for checking if given is string. */
	char *unescaped;
	size_t unescaped_len; /* Length of the unescaped text. */
	
	/***** Check opaque. *****/
	if (strncmp(val, OPAQUE_PREFIX, OPAQUE_PREFIX_LEN) == 0) { 
		/*** Verify length (ie, that it is the multiple of the size of an
		 * escaped character). ***/
		if (len % ESCAPED_LEN != 0) {
			return SLP_PARAMETER_BAD;
		}
		unescaped_len = (len / ESCAPED_LEN) - 1; /* -1 to drop the OPAQUE_PREFIX. */
		
		/*** Verify that every character has been escaped. ***/
		/* TODO */
		
		/***** Unescape the value. *****/
		unescaped = (char *)malloc(unescaped_len);
		if (unescaped == NULL) {
			return SLP_MEMORY_ALLOC_FAILED; /* FIXME: Real error code. */
		}
		
		if (unescape_into(unescaped, (char *)(val + OPAQUE_PREFIX_LEN), len - OPAQUE_PREFIX_LEN) != NULL) {
			SLPError err;
			err = SLPAttrSet_opaque((SLPAttributes)slp_attr, tag, unescaped, (len - OPAQUE_PREFIX_LEN) / 3, policy);
			free(unescaped);/* FIXME This should be put into the val, and free()'d in val_destroy(). */

			return err;
		}
		return SLP_PARAMETER_BAD; /* FIXME Verify. Is this really a bad parameter?*/
	}

	/***** Check boolean. *****/
	if ((BOOL_TRUE_STR_LEN == len) && (strncmp(val, BOOL_TRUE_STR, len) == 0) ) {
		return SLPAttrSet_bool((SLPAttributes)slp_attr, tag, SLP_TRUE);
	}
	if ((BOOL_FALSE_STR_LEN == len) && strncmp(val, BOOL_FALSE_STR, len) == 0) {
		return SLPAttrSet_bool((SLPAttributes)slp_attr, tag, SLP_FALSE);
	}


	/***** Check integer *****/
	if (*val == '-' || isdigit((int)*val)) {
		/*** Verify. ***/
		SLPBoolean is_int = SLP_TRUE; /* Flag true if the attr is an int. */
		for (i = 1; i < len; i++) { /* We start at 1 since first char has already been checked. */
			if (!isdigit((int)val[i])) {
				is_int = SLP_FALSE;
				break;
			} 
		}
		
		/*** Handle the int-ness. ***/
		if (is_int == SLP_TRUE) {
			char *end; /* To verify that the correct length was read. */
			SLPError err;
			err = SLPAttrSet_int((SLPAttributes)slp_attr, tag, strtol(val, &end, 10), policy);
			assert(end == val + len);
			return err;
		}
	}

	/***** Check string. *****/
	is_str = SLP_TRUE;
	for(i = 0; i < len; i++) {
		if (IS_RESERVED(val[i]) && (val[i] != '\\')) {
			is_str = SLP_FALSE;
			break;
		}
	}
	if (is_str == SLP_TRUE) {
		unescaped_len = find_unescaped_size(val, len);
		unescaped = (char *)malloc( unescaped_len + 1 ); 
		if (unescape_into(unescaped, val, len) != NULL) {
			SLPError err;
			unescaped[unescaped_len] = '\0';
			err = SLPAttrSet_str((SLPAttributes)slp_attr, tag, unescaped, policy);
			free(unescaped); /* FIXME This should be put into the val, and free()'d in val_destroy(). */
			return err; 
		}
		
		return SLP_PARAMETER_BAD;
	}


	/* We don't bother checking for a keyword attribute since it can't have a
	 * value. 
	 */

	return SLP_PARAMETER_BAD; /* Could not determine type. */
}



/* Converts an attribute string into an attr struct. 
 *
 * Note that the attribute string is trashed.
 *
 * Returns:
 *  SLP_PARAMETER_BAD -- If there is a parse error in the attribute string. 
 *  SLP_OK -- If everything went okay.
 */
SLPError attr_destringify(
		struct xx_SLPAttributes *slp_attr, 
		char *str, 
		SLPInsertionPolicy policy
) {
	char *cur; /* Current index into str. */
	enum {
		/* Note: everything contained in []s in this enum is a production from
		 * RFC 2608's grammar defining attribute lists. 
		 */
		START_ATTR /* The start of an individual [attribute]. */,
		START_TAG /* The start of a [attr-tag]. */,
		VALUE /* The start of an [attr-val]. */,
		STOP_VALUE /* The end of an [attr-val]. */
	} state = START_ATTR; /* The current state of the parse. */
	char *tag; /* A tag that has been parsed. (carries data across state changes)*/
	assert(str != NULL);
	if (strlen(str) == 0) {
		return SLP_OK;
	}
	
	tag = NULL;
	cur = str;
	/***** Pull apart str. *****/
	while (*cur) {
		char *end; /* The end of a parse entity. */
		switch (state) {
			case(START_ATTR): /* At the beginning of an attribute. */
				if (strncmp(cur, VAR_PREFIX, VAR_PREFIX_LEN) == 0) {
					/* At the start of a non-keyword. */
					state = START_TAG;
					cur += VAR_PREFIX_LEN;
				} else {
					/* At the start of a keyword:
					 * Gobble up the keyword and set it. 
					 */
					end = find_tag_end(cur);
					
					if (end == NULL) {
						/* FIXME Ummm, I dunno. */
						assert(0);
					}
					/*** Check that the tag ends on a legal ending char. ***/
					if (*end == ',') {
						/** Add the keyword. **/
						*end = '\0';
						SLPAttrSet_keyw((SLPAttributes)slp_attr, cur);
						cur = end + 1;
						break;
					}
					else if (*end == '\0') {
						SLPAttrSet_keyw((SLPAttributes)slp_attr, cur);
						return SLP_OK; /* FIXME Return success. */
						break;
					} 
					else {
						return SLP_PARAMETER_BAD; /* FIXME Return error code. -- Illegal tag char */
					}
				}
				break;
			case(START_TAG): /* At the beginning of a tag, in brackets. */
				end = find_tag_end(cur);
				
				if (end == NULL) {
					return SLP_PARAMETER_BAD; /* FIXME Err. code -- Illegal char. */
				}
				
				if (*end == '\0') {
					return SLP_PARAMETER_BAD; /* FIXME err: Premature end. */
				}
				
				/*** Check the the end character is valid. ***/
				if (strncmp(end, VAR_INFIX, VAR_INFIX_LEN) == 0) {
					size_t len = end - cur; /* Note that end is on the character _after_ the last character of the tag (the =). */
					assert(tag == NULL);
					tag = (char *)malloc(len + 1); /* TODO This is incorporated into the corresponding attribute. It is therefore free()'d in the var_free(). */
					strncpy(tag, cur, len);
					tag[len] = '\0';
					cur = end + VAR_INFIX_LEN;
					state = VALUE;
				} 
				else {
					/** ERROR! **/
					return SLP_PARAMETER_BAD; /* FIXME Err. code.-- Logic error. */
				}
				
				break;
				
			case(VALUE): /* At the beginning of the value portion. */
				assert(tag != NULL); /* We should not be able to get into this state is the string is malformed. */
				
				/*** Find the end of the value. ***/
				end = find_value_end(cur);
				
				/*** Check the validity of the end chararcter. */
				if ((strncmp(end, VAR_SUFFIX, VAR_SUFFIX_LEN) == 0) 
					|| strncmp(end, VAR_SEPARATOR, VAR_SEPARATOR_LEN) == 0 ) {
			
					SLPAttrStore(slp_attr, tag, cur, end - cur, policy);
					
					cur = end;
					state = STOP_VALUE;
				} else {
					/*** ERROR! ***/
					return SLP_PARAMETER_BAD; /* FIXME err -- invalid value terminator. */
				}
				break;
			case(STOP_VALUE): /* At the end of a value. */
				/***** Check to see which state we should move into next.*****/
				/*** Done? ***/
				if (*cur == '\0') {
					return SLP_OK;
				}
				/*** Another value? (ie, we're in a value list) ***/
				else if (strncmp(cur, VAR_SEPARATOR, VAR_SEPARATOR_LEN)==0) {
					cur += VAR_SEPARATOR_LEN;
					state = VALUE;
				}
				/*** End of the attribute? ***/
				else if (strncmp(cur, VAR_SUFFIX, VAR_SUFFIX_LEN) == 0) {
					assert(tag != NULL);
					free(tag);
					tag = NULL;
					cur += VAR_SUFFIX_LEN;
					
					/*** Are we at string end? ***/
					if (*cur == '\0') {
						return SLP_OK;
					}
					
					/*** Ensure that there is a seperator ***/
					if (strncmp(cur, VAR_SEPARATOR, VAR_SEPARATOR_LEN) != 0) {
						return  SLP_PARAMETER_BAD; /* FIXME err -- unexpected character. */
					}
					
					cur += VAR_SEPARATOR_LEN;
					state = START_ATTR;
				}
				/*** Error. ***/
				else {
					return SLP_PARAMETER_BAD; /* FIXME err -- Illegal char at value end. */
				}
				break;
			default:
				printf("Unknown state %d\n", state);
		}
	}

	return SLP_OK;
}

void destringify(SLPAttributes slp_attr_h, const char *str) {
	attr_destringify((struct xx_SLPAttributes*)slp_attr_h, (char *)str, SLP_ADD);
}


/* Adds the tags named in attrs to the receiver. Note that the new attributes 
 * _replace_ the old ones.
 *
 * Returns:
 *  SLP_OK -- Update occured as expected.
 *  SLP_MEMORY_ALLOC_FAILED -- Guess.
 *  SLP_PARAMETER_BAD -- Syntax error in the attribute string. Although 
 *  	slp_attr_h is still valid, its contents may have arbitrarily changed. 
 */
SLPError SLPAttrFreshen(SLPAttributes slp_attr_h, const char *str) {
	SLPError err;
	struct xx_SLPAttributes *slp_attr = (struct xx_SLPAttributes*)slp_attr_h;

	char *mangle; /* A copy of the passed in string, since attr_destringify tends to chew data. */
	
	mangle = strdup(str);
	if (str == NULL) {
		return SLP_MEMORY_ALLOC_FAILED;
	}
	err = attr_destringify(slp_attr, mangle, SLP_ADD);
	free(mangle);

	return err;
}


/******************************************************************************
 *
 *                          SLP Control Functions
 *
 *****************************************************************************/

/* Register attributes. */
//SLPError SLPRegAttr( 
//	SLPHandle slp_h, 
//	const char* srvurl, 
//	unsigned short lifetime, 
//	const char* srvtype, 
//	SLPAttributes attr_h, 
//	SLPBoolean fresh, 
//	SLPRegReport callback, 
//	void* cookie 
//) {
//	char *str;
//	SLPError err;
//	
////	err = SLPAttrSerialize(attr_h, size_t *count, char **str, SLPBoolean find_delta); 
//
//	if (str == NULL) {
//		return SLP_INTERNAL_SYSTEM_ERROR;
//	}
//
//	err = SLPReg(slp_h, srvurl, lifetime, srvtype, str, fresh, callback, cookie);
//
//	free(str);
//
//	return err;
//}


struct hop_attr {
	SLPAttrObjCallback *cb;
	void *cookie;
};


SLPBoolean attr_callback (
		SLPHandle hslp, 
		const char* attrlist, 
		SLPError errcode, 
		void* cookie 
) {
	struct hop_attr *hop = (struct hop_attr *)cookie;
	SLPAttributes attr;
	SLPBoolean result;

	assert(errcode == SLP_OK || errcode == SLP_LAST_CALL);
	
	if (errcode == SLP_OK) {
		if (SLPAttrAlloc("en", NULL, SLP_FALSE, &attr) != SLP_OK) {
			/* FIXME Ummm, should prolly tell application about the internal
			 * error.  
			 */
			return SLP_FALSE;
		}
	
		destringify(attr, attrlist);
		result = hop->cb(hslp, attr, errcode, hop->cookie);
		assert(result == SLP_TRUE || result == SLP_FALSE);
		SLPAttrFree(attr);
	} 
	else if (errcode == SLP_LAST_CALL) {
		result = hop->cb(hslp, NULL, errcode, hop->cookie);
	}
	else {
		result = hop->cb(hslp, NULL, errcode, hop->cookie);
	}

	return result;
}


/* Find the attributes of a given service. */
//SLPError SLPFindAttrObj(
//		SLPHandle hslp, 
//		const char* srvurlorsrvtype, 
//		const char* scopelist, 
//		const char* attrids, 
//		SLPAttrObjCallback *callback, 
//		void* cookie
//) {
//	struct hop_attr *hop;
//	SLPError err;
//	
//	hop = (struct hop_attr*)malloc(sizeof(struct hop_attr));
//	hop->cb = callback;
//	hop->cookie = cookie;
//
//	err = SLPFindAttrs(hslp, srvurlorsrvtype, scopelist, attrids, attr_callback, hop);
//
//	free(hop);
//	
//	return err;
//}


/******************************************************************************
 *
 *                                Iterators
 *
 *****************************************************************************/

/* An iterator to make for easy looping across the struct. */
struct xx_SLPAttrIterator {
	int element_count; /* Number of elements. */
	char **tags; /* Array of tags. */
	int current; /* Current index into the attribute iterator. */
	struct xx_SLPAttributes *slp_attr;
};



/* Allocates a new iterator for the given attribute handle. */
SLPError SLPAttrIteratorAlloc(SLPAttributes attr_h, SLPAttrIterator *iter_h) {
	struct xx_SLPAttrIterator *iter;
	struct xx_SLPAttributes *slp_attr = (struct xx_SLPAttributes *)attr_h;
	var_t *var;
	int i;

	assert(slp_attr != NULL);

	iter = (struct xx_SLPAttrIterator *)malloc(sizeof(struct xx_SLPAttrIterator)); /* free()'d in SLPAttrIteratorFree(). */
	if (iter == NULL) {
		return SLP_MEMORY_ALLOC_FAILED;
	}

	iter->element_count = (int)slp_attr->attr_count;
	iter->current = -1;
	iter->slp_attr = slp_attr;

	iter->tags = (char **)malloc(sizeof(char *) * iter->element_count);
	if (iter->tags == NULL) {
		free(iter);
		return SLP_MEMORY_ALLOC_FAILED;
	}
	
	var = slp_attr->attrs;

	for (i = 0; i < iter->element_count; i++, var = var->next) {
		assert(var != NULL);
		
		iter->tags[i] = strdup(var->tag);
		
		/***** Check that strdup succeeded. *****/
		if (iter->tags[i] == NULL) {
			/**** Unallocate structure. ****/
			int up_to_i;
			/*** Unallocate the tag list members. ***/
			for (up_to_i = 0; up_to_i < i; up_to_i++) {
				free(iter->tags[up_to_i]);
			}
			
			/*** Unallocate the tag list ***/
			free(iter->tags);
			
			return SLP_MEMORY_ALLOC_FAILED;
		}
	}

	*iter_h = (SLPAttrIterator)iter;
	
	return SLP_OK;
}


/* Dealloc's an iterator and the associated memory. 
 *
 * Everything free()'d here was alloc'd in SLPAttrIteratorAlloc(). 
 */
void SLPAttrIteratorFree(SLPAttrIterator iter_h) {
	struct xx_SLPAttrIterator *iter = (struct xx_SLPAttrIterator*)iter_h;
	int i;

	/***** Free the tag list. *****/
	for(i = 0; i < iter->element_count; i++) {
		free(iter->tags[i]);
		iter->tags[i] = NULL;
	}
	
	free(iter->tags);
	
	free(iter); 
}


/* Gets the next tag name (and type). 
 *
 * Note: The value of tag _must_ be copied out before the next call to 
 * 	SLPAttrIterNext(). In other words, DO NOT keep pointers to the tag string 
 * 	after the next call to SLPAttrIterNext().
 *
 * Returns SLP_FALSE if there are no tags left to iterate over, or SLP_TRUE.
 */
SLPBoolean SLPAttrIterNext(SLPAttrIterator iter_h, char const **tag, SLPType *type) {
	struct xx_SLPAttrIterator *iter = (struct xx_SLPAttrIterator*)iter_h;
	SLPError err;

	iter->current++;
	if (iter->current >= iter->element_count) {
		*tag = NULL;
		return SLP_FALSE; /* FIXME Return Done. */
	}
	*tag = iter->tags[iter->current];
    err = SLPAttrGetType(iter->slp_attr, *tag, type);

	if (err != SLP_OK) {
		return SLP_FALSE; /* FIXME Ummm, try to get the next one. */
	}
	
	return SLP_TRUE;
}




