/*
 * 	BMR_BGW_aux.h
 * 
 *      Author: Aner Ben-Efraim 
 * 	
 * 	year: 2016
 * 
 */

#ifndef _BMR_BGW_AUX_H_
#define _BMR_BGW_AUX_H_
#pragma once

#include <stdio.h> 
#include <iostream>
#include "Config.h"
#include "../util/TedKrovetzAesNiWrapperC.h"
#include <wmmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include <vector>
#include <time.h>
#include "secCompMultiParty.h"
#include "main_gf_funcs.h"

#if MULTIPLICATION_TYPE==0
#define MUL(x,y) gfmul(x,y)
#define MULT(x,y,ans) gfmul(x,y,&ans)

#ifdef OPTIMIZED_MULTIPLICATION
#define MULTHZ(x,y,ans) gfmulHalfZeros(x,y,&ans)//optimized multiplication when half of y is zeros
#define MULHZ(x,y) gfmulHalfZeros(x,y)//optimized multiplication when half of y is zeros
#else
#define MULTHZ(x,y,ans) gfmul(x,y,&ans)
#define MULHZ(x,y) gfmul(x,y)
#endif
#define SET_ONE _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1)

#else 
#define MUL(x,y) gfmul3(x,y)
#define MULT(x,y,ans) gfmul3(x,y,&ans)
#ifdef OPTIMIZED_MULTIPLICATION
#define MULTHZ(x,y,ans) gfmul3HalfZeros(x,y,&ans)//optimized multiplication when half of y is zeros
#define MULHZ(x,y) gfmul3HalfZeros(x,y)//optimized multiplication when half of y is zeros
#else
#define MULTHZ(x,y,ans) gfmul3(x,y,&ans)
#define MULHZ(x,y) gfmul3(x,y)
#endif 
#define SET_ONE _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1)
#endif 

// 
//field zero
#define SET_ZERO _mm_setzero_si128()
//the partynumber(+1) embedded in the field
#define SETX(j) _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, j+1)//j+1
//Test if 2 __m128i variables are equal
#define EQ(x,y) _mm_test_all_ones(_mm_cmpeq_epi8(x,y))
//Add 2 field elements in GF(2^128)
#define ADD(x,y) _mm_xor_si128(x,y)
//Subtraction and addition are equivalent in characteristic 2 fields
#define SUB ADD
//Evaluate x^n in GF(2^128)
#define POW(x,n) fastgfpow(x,n)
//Evaluate x^2 in GF(2^128)
#define SQ(x) square(x)
//Evaluate x^(-1) in GF(2^128)
#define INV(x) inverse(x)
//Evaluate P(x), where p is a given polynomial, in GF(2^128)
#define EVAL(x,poly,ans) fastPolynomEval(x,poly,&ans)//polynomEval(SETX(x),y,z)
//Reconstruct the secret from the shares
#define RECON(shares,deg,secret) reconstruct(shares,deg,&secret)
//returns a (pseudo)random __m128i number using AES-NI
#define RAND LoadSeedNew()
//returns a (pseudo)random bit using AES-NI
#define RAND_BIT LoadBool()

//the encryption scheme
#define PSEUDO_RANDOM_FUNCTION(seed1, seed2, index, numberOfBlocks, result) fixedKeyPseudoRandomFunctionwPipelining(seed1, seed2, index, numberOfBlocks, result);

//The degree of the secret-sharings before multiplications
extern int degSmall;
//The degree of the secret-sharing after multiplications (i.e., the degree of the secret-sharings of the PRFs)
extern int degBig;
//The type of honest majority we assume
extern int majType;

//bases for interpolation
extern __m128i* baseReduc;
extern __m128i* baseRecon;
//saved powers for evaluating polynomials
extern __m128i* powers;

//one in the field
extern const __m128i ONE;
//zero in the field
extern const __m128i ZERO;

extern int testCounter;

typedef struct polynomial {
	int deg;
	__m128i* coefficients;
}Polynomial;


void gfmul(__m128i a, __m128i b, __m128i *res);

//This function works correctly only if all the upper half of b is zeros
void gfmulHalfZeros(__m128i a, __m128i b, __m128i *res);

//multiplies a and b
__m128i gfmul(__m128i a, __m128i b);

//This function works correctly only if all the upper half of b is zeros
__m128i gfmulHalfZeros(__m128i a, __m128i b);

__m128i gfpow(__m128i x, int deg);

__m128i fastgfpow(__m128i x, int deg);


__m128i square(__m128i x);

__m128i inverse(__m128i x);


void fastPolynomEval(int x, Polynomial poly, __m128i* ans);

__m128i polynomEval(__m128i x, __m128i* coefficients, int deg);

//using precomputed powers to save time
void fastPolynomEval(int x, __m128i* coefficients, int deg, __m128i* ans);

//computes a single coefficient for the lagrange polynomial
void computeB(int party, int numOfParties, __m128i* ans);

//precomputes powers and the coefficients for the lagrange polynomial - should be done once in setup
void preComputeCoefficients(int numOfParties, int deg);//degree should be the degree of polynomial which we reduce from

//precomputes the coefficients for the lagrange polynomial. Not needed in most versions since degree of reconstruction === degree of reduction
void preComputeReconstructionCoefficients(int numOfParties,int deg);//degree should be the degree of reconstruction

//reconstructs the secret from the shares
void reconstruct(__m128i* points, int deg, __m128i* secret);
//reconstructs the secret from the shares
__m128i reconstruct(__m128i* points, int deg);

__m128i* createShares(__m128i secret, int numOfParties, int deg);


/*
This function can be used in 2 ways:
1: a and b are shares, then get share of ab (used in Step I of BGW)
2: b is a share and a is a constant, then share of ab (used in Step II of BGW)
*/
__m128i mulShares(__m128i a, __m128i b);

/*
This function can be used in 2 ways:
1: a and b are shares, then get share of ab (used in Step I of BGW)
2: b is a share and a is a constant, then share of ab (used in Step II of BGW)
*/
void mulShares(__m128i a, __m128i b, __m128i* ab);

//creates share for BGW round
__m128i* BGWshares(__m128i shareA, __m128i shareB, int numOfParties, int deg);//int party


//Won't be used probably, cause it will more efficient to do it without a separate function
__m128i BGWrecoverShare(__m128i* BGWshares, int numOfParties);


inline __m128i* mulShares(__m128i* a, __m128i* b, int length)
{
	__m128i* ans = static_cast<__m128i *>(_aligned_malloc(length*sizeof(__m128i), 16));
	for (int i = 0; i < length; i++)
	{
		MULT(a[i], b[i], ans[i]);
	}
	return ans;
}



#endif