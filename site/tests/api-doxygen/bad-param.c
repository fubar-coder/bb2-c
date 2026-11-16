/* A deliberately broken comment for the test api_doxygen_broken: Doxygen
   must fail with "argument 'x' ... is not found". */

/**
 * @brief An assembler entry point without C parameters.
 *
 * @param x d0: no C parameter of this name
 */
void bad_param(void);
