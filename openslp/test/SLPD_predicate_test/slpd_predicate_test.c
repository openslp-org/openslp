
#include <assert.h>

#include <slpd_predicate.h>
#include <libslpattr.h>

#ifdef USE_PREDICATES

SLPError SLPAttrEvalPred(SLPAttributes slp_attr, SLPDPredicate *predicate, SLPBoolean *result, int recursion_depth); /* It's defined in slpd_predicate.c, but since we don't want to pollute namespaces, we put the prototype here. -- we use this so we don't have to calculate the string length, and so we can play with the recursion depth.*/

int main(int argc, char *argv[]) {
	char *str;
	int ierr;
	SLPAttributes slp_attr;
	SLPError err;
	SLPBoolean result;

	
	/******************** Test int stuff. *********************/
	
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &slp_attr);
	assert(err == SLP_OK);
	
	SLPAttrSet_int(slp_attr, "int", (long)23, SLP_ADD);
	SLPAttrSet_int(slp_attr, "int", (long)25, SLP_ADD);
	SLPAttrSet_int(slp_attr, "int", (long)27, SLP_ADD);
	
	/* Test equals. */
	str = "(&(&(int=23)(int=25))(int=26))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* False. */
	
	str = "(&(&(int=24)(int=25))(int=26))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* False. */
	
	str = "(&(&(int=24)(int=28))(int=26))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* False. */

	str = "(&(&(int=23)(int=25))(int=27))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* True. */


	/* Test greater. */
	str = "(int>=29)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f. */
	
	str = "(int>=26)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* T. */
	
	str = "(int>=24)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t. */
	
	str = "(int>=22)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t. */

	
	/* Test lesser. */
	str = "(int<=22)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */
	
	str = "(int<=23)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */
	
	SLPAttrFree(slp_attr);

	/* Simple equality. */
	err = SLPAttrAllocStr("en", NULL, SLP_FALSE, &slp_attr, "(a=1)");
	assert(err == SLP_OK);

	str = "(a=1)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */
	
	SLPAttrFree(slp_attr);
	/******************** Test string stuff. *********************/
	
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &slp_attr);
	assert(err == SLP_OK);
	
	err = SLPAttrSet_str(slp_attr, "str", "string", SLP_REPLACE);
	assert(err == SLP_OK);


	/* Test less (single-valued). */
	str = "(str<=a)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */
	
	str = "(str<=string)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */
	
	str = "(str<=strinx)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */


	/* Test greater (single-valued). */
	str = "(str>=a)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */
	
	str = "(str>=string)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */
	
	str = "(str>=strinx)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */


	/* Test equal (single valued). */
	str = "(str=a)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */
	
	str = "(str=*ing)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */
	
	str = "(str=stri*)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */
	
	str = "(str=*tri*)";
	ierr = SLPTestPredicate( str, slp_attr);
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
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(bool=false)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	/* Test bad strings. */
	str = "(bool=falsew)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(bool=*false)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(bool=truee)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(bool= true)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	
	SLPAttrFree(slp_attr);
	
	
	/******************** Test keyword stuff. *********************/
	
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &slp_attr);
	assert(err == SLP_OK);
	
	err = SLPAttrSet_keyw(slp_attr, "keyw");
	assert(err == SLP_OK);

	/* Test present. */
	str = "(keyw=*)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(keyw=sd)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(keyw<=adf)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	SLPAttrFree(slp_attr);

	/********************* Test boolean operators. *********************/
	err = SLPAttrAlloc("en", NULL, SLP_FALSE, &slp_attr);
	assert(err == SLP_OK);
	
	err = SLPAttrSet_keyw(slp_attr, "keyw");
	assert(err == SLP_OK);


	str = "(keyw=*)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	/* Test not. */
	str = "(!(keyw=*))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(!(!(keyw=*)))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(!(!(!(keyw=*))))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(!(!(!(!(keyw=*)))))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	/* Build up to testing binary ops. */
	err = SLPAttrSet_bool(slp_attr, "bool", SLP_TRUE);
	assert(err == SLP_OK);
	
	/* Test and. */
	str = "(&(keyw=*)(bool=true))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(&(keyw=*)(bool=false))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(&(keyw=*)(!(bool=false)))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(&(keywx=*)(bool=true))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(&(!(keywx=*))(bool=true))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(&(lkeyw=*)(bool=false))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(&(!(lkeyw=*))(!(bool=false)))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(&(&(keyw=*)(bool=true))(&(keyw=*)(bool=true)))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	str = "(&(&(!(keyw=*))(bool=true))(&(keyw=*)(bool=true)))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */

	str = "(!(&(&(!(keyw=*))(bool=true))(&(keyw=*)(bool=true))))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr == 0); /* t */

	
	/* Test sytax errors. */

	/* No preceeding bracket. */
	str = "asdf=log";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr < 0);

	/* No trailing bracket. */
	str = "(asdf=log";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr < 0);

	/* Unbalanced brackets. */
	str = "(asdf=log))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr < 0);

	str = "((asdf=log)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr < 0);

	/* Missing operators. */
	str = "(asdflog)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr < 0);

	/* Check that the leaf operator isn't causing the problem. */
	str = "(asdflog=q)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0);

	/* Missing logical unary. */
	str = "((asdflog=q))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr < 0);

	/* Missing logical binary. */
	str = "((asdflog=q)(asdflog=q))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr < 0);

	/* Missing operands and operator. */
	str = "()";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr < 0);
	
	/* Missing unary operands. */
	str = "(!)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr < 0);
	
	/* Missing binary operands. */
	str = "(&)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr < 0);
	
	/* Missing binary operands. */
	str = "(=)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr < 0);
	
	/* Missing binary operands. I _guess_ this is legal... */
	str = "(thingy=)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > -1);

	/* Trailing trash. */
	str = "(&(a=b)(c=d))";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > -1);

	/* Check that the following test will not be short circuited. */
	str = "(a=b)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr > 0); /* f */
	
	str = "(|(a=b)(c=d)w)";
	ierr = SLPTestPredicate( str, slp_attr);
	assert(ierr < 0);

	/* Check recursion depth. */
	str = "(a=b)";
	ierr = SLPAttrEvalPred(slp_attr, (SLPDPredicate)str, &result, 1);
	assert(ierr == 0); /* f */
	
	str = "(!(a=b))";
	ierr = SLPAttrEvalPred(slp_attr, (SLPDPredicate)str, &result, 1);
	assert(ierr < 0);

	/* Check that short-circuiting works. */
	str = "(&(a=b)(!(a=b)))";
	ierr = SLPAttrEvalPred(slp_attr, (SLPDPredicate)str, &result, 2);
	assert(ierr == 0); /* f */

	str = "(|(a=b)(!(a=b)))";
	ierr = SLPAttrEvalPred(slp_attr, (SLPDPredicate)str, &result, 2);
	assert(ierr < 0);

	str = "(|(keyw=*)(!(a=b)))";
	ierr = SLPAttrEvalPred(slp_attr, (SLPDPredicate)str, &result, 2);
	assert(ierr == 0); /* t */

	
	SLPAttrFree(slp_attr);
	
	
	return 0;
}

#else /* USE_PREDICATES */

int main(int argc, char *argv[]) {
	puts("Predicates disabled. Not testing.");
	return 1;
}

#endif /* USE_PREDICATES */
