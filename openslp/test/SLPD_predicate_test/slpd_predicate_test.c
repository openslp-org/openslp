#include <assert.h>
#include <string.h>

#include <libslpattr.h>

#ifdef USE_PREDICATES

typedef void* SLPDPredicate; 
int SLPDPredicateTest(SLPDPredicate predicate, SLPAttributes attr);

typedef enum
{
	FR_UNSET /* Placeholder. Used to detect errors. */, 
	FR_INTERNAL_SYSTEM_ERROR /* Internal error. */,
	FR_PARSE_ERROR /* Parse error detected. */, 
	FR_MEMORY_ALLOC_FAILED /* Ran out of memory. */, 
	FR_EVAL_TRUE /* Expression successfully evaluated to true. */, 
	FR_EVAL_FALSE /* Expression successfully evaluated to false. */
} FilterResult;

FilterResult wildcard(const char *pattern, int pattern_len, const char *str, int str_len);

#define ez_WILDCARD(x,y) wildcard(x, (int) strlen(x), y, (int) strlen(y))

void test_wildcard() {
#ifdef USE_PREDICATES
	int err;

	err = ez_WILDCARD("slug", "slug");
	assert(err == FR_EVAL_TRUE);

	err = ez_WILDCARD("slug*", "slug");
	assert(err == FR_EVAL_TRUE);

	err = ez_WILDCARD("slug*", "slugx");
	assert(err == FR_EVAL_TRUE);

	err = ez_WILDCARD("slug*", "slugxy");
	assert(err == FR_EVAL_TRUE);

	err = ez_WILDCARD("slug*y", "slugxy");
	assert(err == FR_EVAL_TRUE);

	err = ez_WILDCARD("slug*x", "slugxy");
	assert(err == FR_EVAL_FALSE);

	err = ez_WILDCARD("s*y", "slugxy");
	assert(err == FR_EVAL_TRUE);

	err = ez_WILDCARD("sl*xy", "slugxy");
	assert(err == FR_EVAL_TRUE);

	err = ez_WILDCARD("ab*ab", "ababcdab");
	assert(err == FR_EVAL_TRUE);

	err = ez_WILDCARD("ab*ab*", "ababcdab");
	assert(err == FR_EVAL_TRUE);

	err = ez_WILDCARD("*", "ababcdab");
	assert(err == FR_EVAL_TRUE);

	err = ez_WILDCARD("*cd", "ababcdab");
	assert(err == FR_EVAL_FALSE);

	err = ez_WILDCARD("*cdab", "ababcdab");
	assert(err == FR_EVAL_TRUE);

	err = ez_WILDCARD("*cd*", "ababcdab");
	assert(err == FR_EVAL_TRUE);

	err = ez_WILDCARD("*c*d*", "ababcdab");
	assert(err == FR_EVAL_TRUE);

	err = ez_WILDCARD("*****c****d****", "ababcdab");
	assert(err == FR_EVAL_TRUE);

	/* Test escaping. */
	err = ez_WILDCARD("ab\\2Acd", "ab*cd");
	assert(err == FR_EVAL_TRUE);

	/* Test escaping. */
	err = ez_WILDCARD("ab\\2A\\2A", "ab**");
	assert(err == FR_EVAL_TRUE);

	/* Test escaping. */
	err = ez_WILDCARD("ab\\2A\\2Aln*", "ab**lnas");
	assert(err == FR_EVAL_TRUE);

	/* Test escaping. */
	err = ez_WILDCARD("ab\\2A\\2Aln", "ab**lnas");
	assert(err == FR_EVAL_FALSE);

	/* Test escaping. */
	err = ez_WILDCARD("ab\\2A*\\2Aln", "ab*x*l*ln");
	assert(err == FR_EVAL_TRUE);

#else /* USE_PREDICATES */
	puts("Predicates disabled. Skipping.");
#endif /* USE_PREDICATES */

}

void test_predicate() {
	char *str;
	int ierr;
	SLPAttributes slp_attr;
	SLPError err;

	
	/******************** Test int stuff. *********************/
	
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &slp_attr);
	assert(err == SLP_OK);
	
	SLPAttrSet_int(slp_attr, "int", (int)23, SLP_ADD);
	SLPAttrSet_int(slp_attr, "int", (int)25, SLP_ADD);
	SLPAttrSet_int(slp_attr, "int", (int)27, SLP_ADD);
	
	/* Test equals. */
	str = "(&(&(int=23)(int=25))(int=26))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* False. */
	
	str = "(&(&(int=24)(int=25))(int=26))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* False. */
	
	str = "(&(&(int=24)(int=28))(int=26))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* False. */

	str = "(&(&(int=23)(int=25))(int=27))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* True. */


	/* Test greater. */
	str = "(int>=29)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f. */
	
	str = "(int>=26)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* T. */
	
	str = "(int>=24)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t. */
	
	str = "(int>=22)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t. */

	
	/* Test lesser. */
	str = "(int<=22)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */
	
	str = "(int<=23)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */
	
	SLPAttrFree(slp_attr);

	/* Simple equality. */
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &slp_attr, "(a=1)");
	assert(err == SLP_OK);

	str = "(a=1)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */
	
	SLPAttrFree(slp_attr);


	/******************** Test opaque stuff. *********************/
	
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &slp_attr);
	assert(err == SLP_OK);
	
	err = SLPAttrSet_str(slp_attr, "op", "\\00\\12\\24\\36", SLP_REPLACE);
	assert(err == SLP_OK);


	/* Test less (single-valued). */
	str = "(op<=\\00\\12\\10\\43)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */
	
	str = "(op<=\\00\\12\\24\\36)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */
	
	str = "(op<=\\00\\12\\24\\36\\12)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */
	
	str = "(op>=\\00\\12\\24\\36)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	SLPAttrFree(slp_attr);

	
	/******************** Test string stuff. *********************/
	
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &slp_attr);
	assert(err == SLP_OK);
	
	err = SLPAttrSet_str(slp_attr, "str", "string", SLP_REPLACE);
	assert(err == SLP_OK);


	/* Test less (single-valued). */
	str = "(str<=a)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */
	
	str = "(str<=string)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */
	
	str = "(str<=strinx)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */


	/* Test greater (single-valued). */
	str = "(str>=a)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */
	
	str = "(str>=string)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */
	
	str = "(str>=strinx)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */


	/* Test equal (single valued). */
	str = "(str=a)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */
	
	str = "(str=*ing)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */
	
	str = "(str=stri*)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */
	
	str = "(str=*tri*)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(str=\\73*)"; /* s* */
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(str=\\73\\74\\72\\69*)"; /* stri* */
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(str=*\\73\\74\\72\\69*)"; /* *stri* */
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(str=s*t*r*i*n*g)"; /* s*t*r*i* */
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(str=s*t*r*i*ng)"; /* s*t*r*i* */
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(str=\\73\\74\\72\\69ng)"; /* s*t*r*i* */
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(str=s*tring)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(str=\\73*\\74ring)"; /* s*t*r*i* */
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */


	/* TODO Test escaped '*'s. */
	/* TODO Test multivalued. */
	
	SLPAttrFree(slp_attr);
	
	/******************** Test boolean stuff. *********************/
	
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &slp_attr);
	assert(err == SLP_OK);
	
	err = SLPAttrSet_bool(slp_attr, "bool", SLP_TRUE);
	assert(err == SLP_OK);
	

	/* Test equal. */
	str = "(bool=true)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(bool=false)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */

	/* Test bad strings. */
	str = "(bool=falsew)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(bool=*false)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(bool=truee)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(bool= true)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */

	
	SLPAttrFree(slp_attr);
	
	
	/******************** Test keyword stuff. *********************/
	
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &slp_attr);
	assert(err == SLP_OK);
	
	err = SLPAttrSet_keyw(slp_attr, "keyw");
	assert(err == SLP_OK);

	/* Test present. */
	str = "(keyw=*)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(keyw=sd)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(keyw<=adf)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */

	SLPAttrFree(slp_attr);

	/********************* Test boolean operators. *********************/
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &slp_attr);
	assert(err == SLP_OK);
	
	err = SLPAttrSet_keyw(slp_attr, "keyw");
	assert(err == SLP_OK);


	str = "(keyw=*)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	/* Test not. */
	str = "(!(keyw=*))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(!(!(keyw=*)))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(!(!(!(keyw=*))))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(!(!(!(!(keyw=*)))))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	/* Build up to testing binary ops. */
	err = SLPAttrSet_bool(slp_attr, "bool", SLP_TRUE);
	assert(err == SLP_OK);
	
	/* Test and. */
	str = "(&(keyw=*)(bool=true))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(&(keyw=*)(bool=false))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(&(keyw=*)(!(bool=false)))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(&(keywx=*)(bool=true))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(&(!(keywx=*))(bool=true))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(&(lkeyw=*)(bool=false))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(&(!(lkeyw=*))(!(bool=false)))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(&(&(keyw=*)(bool=true))(&(keyw=*)(bool=true)))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(&(&(!(keyw=*))(bool=true))(&(keyw=*)(bool=true)))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(!(&(&(!(keyw=*))(bool=true))(&(keyw=*)(bool=true))))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr == 0); /* t */

	
	/* Test sytax errors. */

	/* No preceeding bracket. */
	str = "asdf=log";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr < 0);

	/* No trailing bracket. */
	str = "(asdf=log";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr < 0);

	/* Unbalanced brackets. */
	str = "(asdf=log))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr < 0);

	str = "((asdf=log)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr < 0);

	/* Missing operators. */
	str = "(asdflog)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr < 0);

	/* Check that the leaf operator isn't causing the problem. */
	str = "(asdflog=q)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0);

	/* Missing logical unary. */
	str = "((asdflog=q))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr < 0);

	/* Missing logical binary. */
	str = "((asdflog=q)(asdflog=q))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr < 0);

	/* Missing operands and operator. */
	str = "()";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr < 0);
	
	/* Missing unary operands. */
	str = "(!)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr < 0);
	
	/* Missing binary operands. */
	str = "(&)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr < 0);
	
	/* Missing binary operands. */
	str = "(=)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr < 0);
	
	/* Missing binary operands. I _guess_ this is legal... */
	str = "(thingy=)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > -1);

	/* Trailing trash. */
	str = "(&(a=b)(c=d))";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > -1);

	/* Check that the following test will not be short circuited. */
	str = "(a=b)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr > 0); /* f */
	
	str = "(|(a=b)(c=d)w)";
	ierr = SLPDPredicateTest( str, slp_attr);
	assert(ierr < 0);

	SLPAttrFree(slp_attr);


	/* Check multiple (more than two) subexpressions. */
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &slp_attr);
	assert(err == SLP_OK);

	err = SLPAttrSet_int(slp_attr, "x", 1, SLP_ADD);
	assert(err == SLP_OK);
	
	str = "(&(x=1)(!(x=1)))";
	ierr = SLPDPredicateTest((SLPDPredicate)str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(&(x=1))";
	ierr = SLPDPredicateTest((SLPDPredicate)str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(&(x=1)(x=1)(x=1))";
	ierr = SLPDPredicateTest((SLPDPredicate)str, slp_attr);
	assert(ierr == 0); /* t */

	SLPAttrFree(slp_attr);
}

int main(int argc, char *argv[]) {
	test_predicate();
	test_wildcard();

	return 0;
}


#else /* USE_PREDICATES */

int main(int argc, char *argv[]) {
	puts("Predicates disabled. Not testing.");
	return 0;
}

#endif /* USE_PREDICATES */
