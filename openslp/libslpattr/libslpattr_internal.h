/*****
 *  Abstract:
 *
 *  The internal structures that represent an attribute list. This is made 
 *  available for slpd to speed its access to values for predicate evaluation. 
 ****/

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

	/* Memory handling */
	struct xx_value_t *next_chunk; /* The next chunk of allocated memory in the value list. */
	struct xx_value_t *last_value_in_chunk; /* The last value in the chunk. Only set by the chunk head. */
} value_t;


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



/* Finds a variable by its tag. */
var_t *attr_val_find_str(struct xx_SLPAttributes *slp_attr, const char *tag, size_t tag_len); 

/* Finds the type of an attribute. */
SLPError SLPAttrGetType_len(SLPAttributes attr_h, const char *tag, size_t tag_len, SLPType *type); 
