/*
 * 	BMR_BGW_aux.cpp
 * 
 *      Author: Aner Ben-Efraim 
 * 	
 * 	year: 2016
 * 
 */


#include "BMR_BGW_aux.h"
#include <mutex> 

/* Here are the global variables, precomputed once in order to save time*/
//powers[x*deg+(i-1)]=x^i, i.e., POW(SETX(X),i). Notice the (i-1), due to POW(x,0) not saved (always 1...)
__m128i* powers;
//the coefficients used in the reconstruction
__m128i* baseReduc;
__m128i* baseRecon;
//allocation of memory for polynomial coefficients
__m128i* polyCoef;
//one in the field
const __m128i ONE = SET_ONE;
//zero in the field
const __m128i ZERO = SET_ZERO;
//the constant used for BGW step by this party
__m128i BGWconst;

__m128i* sharesTest;

void gfmul(__m128i a, __m128i b, __m128i *res) {
	__m128i tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6,
		tmp7, tmp8, tmp9, tmp10, tmp11, tmp12;
	__m128i XMMMASK = _mm_setr_epi32(0xffffffff, 0x0, 0x0, 0x0);
	tmp3 = _mm_clmulepi64_si128(a, b, 0x00);
	tmp6 = _mm_clmulepi64_si128(a, b, 0x11);
	tmp4 = _mm_shuffle_epi32(a, 78);
	tmp5 = _mm_shuffle_epi32(b, 78);
	tmp4 = _mm_xor_si128(tmp4, a);
	tmp5 = _mm_xor_si128(tmp5, b);
	tmp4 = _mm_clmulepi64_si128(tmp4, tmp5, 0x00);
	tmp4 = _mm_xor_si128(tmp4, tmp3);
	tmp4 = _mm_xor_si128(tmp4, tmp6);
	tmp5 = _mm_slli_si128(tmp4, 8);
	tmp4 = _mm_srli_si128(tmp4, 8);
	tmp3 = _mm_xor_si128(tmp3, tmp5);
	tmp6 = _mm_xor_si128(tmp6, tmp4);
	tmp7 = _mm_srli_epi32(tmp6, 31);
	tmp8 = _mm_srli_epi32(tmp6, 30);
	tmp9 = _mm_srli_epi32(tmp6, 25);
	tmp7 = _mm_xor_si128(tmp7, tmp8);
	tmp7 = _mm_xor_si128(tmp7, tmp9);
	tmp8 = _mm_shuffle_epi32(tmp7, 147);

	tmp7 = _mm_and_si128(XMMMASK, tmp8);
	tmp8 = _mm_andnot_si128(XMMMASK, tmp8);
	tmp3 = _mm_xor_si128(tmp3, tmp8);
	tmp6 = _mm_xor_si128(tmp6, tmp7);
	tmp10 = _mm_slli_epi32(tmp6, 1);
	tmp3 = _mm_xor_si128(tmp3, tmp10);
	tmp11 = _mm_slli_epi32(tmp6, 2);
	tmp3 = _mm_xor_si128(tmp3, tmp11);
	tmp12 = _mm_slli_epi32(tmp6, 7);
	tmp3 = _mm_xor_si128(tmp3, tmp12);

	*res = _mm_xor_si128(tmp3, tmp6);
}

//this function works correctly only if all the upper half of b is zeros
void gfmulHalfZeros(__m128i a, __m128i b, __m128i *res) {
	__m128i tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6,
		tmp7, tmp8, tmp9, tmp10, tmp11, tmp12;
	__m128i XMMMASK = _mm_setr_epi32(0xffffffff, 0x0, 0x0, 0x0);
	tmp3 = _mm_clmulepi64_si128(a, b, 0x00);
	tmp4 = _mm_shuffle_epi32(a, 78);
	tmp5 = _mm_shuffle_epi32(b, 78);
	tmp4 = _mm_xor_si128(tmp4, a);
	tmp5 = _mm_xor_si128(tmp5, b);
	tmp4 = _mm_clmulepi64_si128(tmp4, tmp5, 0x00);
	tmp4 = _mm_xor_si128(tmp4, tmp3);
	tmp5 = _mm_slli_si128(tmp4, 8);
	tmp4 = _mm_srli_si128(tmp4, 8);
	tmp3 = _mm_xor_si128(tmp3, tmp5);
	tmp6 = tmp4;
	tmp7 = _mm_srli_epi32(tmp6, 31);
	tmp8 = _mm_srli_epi32(tmp6, 30);
	tmp9 = _mm_srli_epi32(tmp6, 25);
	tmp7 = _mm_xor_si128(tmp7, tmp8);
	tmp7 = _mm_xor_si128(tmp7, tmp9);
	tmp8 = _mm_shuffle_epi32(tmp7, 147);
	
	tmp8 = _mm_andnot_si128(XMMMASK, tmp8);
	tmp3 = _mm_xor_si128(tmp3, tmp8);
	tmp10 = _mm_slli_epi32(tmp6, 1);
	tmp3 = _mm_xor_si128(tmp3, tmp10);
	tmp11 = _mm_slli_epi32(tmp6, 2);
	tmp3 = _mm_xor_si128(tmp3, tmp11);
	tmp12 = _mm_slli_epi32(tmp6, 7);
	tmp3 = _mm_xor_si128(tmp3, tmp12);
	*res = _mm_xor_si128(tmp3, tmp6);
}

//multiplies a and b
__m128i gfmul(__m128i a, __m128i b)
{
	__m128i ans;
	gfmul(a, b, &ans);
	return ans;
}


//This function works correctly only if all the upper half of b is zeros
__m128i gfmulHalfZeros(__m128i a, __m128i b)
{
	__m128i ans;
	gfmulHalfZeros(a, b, &ans);
	return ans;
}

__m128i gfpow(__m128i x, int deg)
{
	__m128i ans = ONE;
	//TODO: improve this
	for (int i = 0; i < deg; i++)
		ans = MUL(ans, x);

	return ans;
}

__m128i fastgfpow(__m128i x, int deg)
{
	__m128i ans = ONE;
	if (deg == 0)
		return ans;
	else if (deg == 1)
		return x;
	else
	{
		int hdeg = deg / 2;
		__m128i temp1 = fastgfpow(x, hdeg);
		__m128i temp2 = fastgfpow(x, deg - hdeg);
		ans = MUL(temp1, temp2);
	}
	return ans;
}

__m128i square(__m128i x)
{
	return POW(x, 2);
}

__m128i inverse(__m128i x)
{
	__m128i powers[128];
	powers[0] = x;
	__m128i ans = ONE;
	for (int i = 1; i < 128; i++)
	{
		powers[i] = SQ(powers[i - 1]);
		ans = MUL(ans, powers[i]);
	}
	return ans;
}

__m128i polynomEval(__m128i x, __m128i* coefficients, int deg)
{
	__m128i ans = coefficients[0];
	for (int i = 1; i < deg + 1; i++)
		ans = ADD(ans, MUL(coefficients[i], POW(x, i)));
	return ans;
}

void fastPolynomEval(int x, Polynomial poly, __m128i* ans)
{
	fastPolynomEval(x, poly.coefficients, poly.deg, ans);
}

//using precomputed powers to save time
void fastPolynomEval(int x, __m128i* coefficients, int deg, __m128i* ans)
{
	ans[0] = coefficients[0];
	for (int i = 1; i < deg + 1; i++)//notice the i-1 due to POW(X,0) not saved (always 1...)
		ans[0] = ADD(ans[0], MUL(coefficients[i], powers[x*degBig + (i - 1)]));//POW(SETX(x), i)));//
}


//computes a single coefficient for the lagrange polynomial
void computeB(int party, int numOfParties, __m128i* ans)
{
	ans[0] = ONE;
	__m128i temp;
	//compute i/(i+j) for all j!=i (i=party) and multiply
	for (int j = 0; j < numOfParties; j++)
		if (j != party)
		{
			MULTHZ(ans[0], SETX(j), ans[0]);
			MULT(ans[0], INV(ADD(SETX(party), SETX(j))),ans[0]);
		}
}

//computes the coefficients for the lagrange polynomial - should be done once in setup
//deg should be the big degree (n-1 in most cases)
void preComputeCoefficients(int numOfParties, int deg)
{

	//percompute base polynomials for lagrange interpolation (for reduction and reconstruction)
	baseReduc = static_cast<__m128i *>(_aligned_malloc((deg + 1)*sizeof(__m128i), 16));
	for (int i = 0; i < deg + 1; i++)
	{
		computeB(i, deg+1, &baseReduc[i]);
	}
	//precompute powers (for polynomial evaluation - secret sharing)
	powers = static_cast<__m128i *>(_aligned_malloc((numOfParties*deg)*sizeof(__m128i), 16));//only deg and not deg+1, because POW(x,0) not saved
	for (int p = 0; p < numOfParties; p++)
	{
		__m128i power = ONE;
		__m128i x = SETX(p);
		for (int i = 1; i < deg + 1; i++)
		{
			MULT(power, x, power);
			powers[p*deg + i - 1] = power;//=POW(x,i);//notice the i-1, due to POW(x,0) not saved (always 1...)
		}
	}
	
	baseRecon = baseReduc;
}

void preComputeReconstructionCoefficients(int numOfParties, int deg)
{
	//percompute base polynomials for lagrange interpolation for reconstruction using small degree - used only in version hm==3
	baseRecon = static_cast<__m128i *>(_aligned_malloc((deg + 1)*sizeof(__m128i), 16));
	for (int i = 0; i < deg + 1; i++)
	{
		computeB(i, deg + 1, &baseRecon[i]);
	}
}

//reconstructs the secret from the shares
void reconstruct(__m128i* points, int deg, __m128i* secret)
{
	secret[0] = ZERO;
	for (int i = 0; i < deg + 1; i++)
		secret[0] = ADD(secret[0], MUL(points[i], baseRecon[i]));
}

//reconstructs the secret from the shares
__m128i reconstruct(__m128i* points, int deg)
{
	__m128i secret = ZERO;
	for (int i = 0; i < deg + 1; i++)
		secret = ADD(secret, MUL(points[i], baseRecon[i]));
	return secret;
}

__m128i* createShares(__m128i secret, int numOfParties, int deg)
{
	//coefficients of randomly chosen poynomial
	if (!polyCoef)//if memory not allocated then allocate memory (might be more than degree, 'cause done only once, so needs to fit all secret-sharings)
	{
		polyCoef = static_cast<__m128i *>(_aligned_malloc((numOfParties)*sizeof(__m128i), 16));
	}
	
	for (int i = 1; i < deg + 1; i++)
		polyCoef[i] = RAND;
	polyCoef[0] = secret;
	//compute shares by evaluating
	sharesTest = static_cast<__m128i *>(_aligned_malloc(numOfParties*sizeof(__m128i), 16));
	for (int p = 0; p < numOfParties; p++)
		fastPolynomEval(p, polyCoef, deg, &sharesTest[p]);

	return sharesTest;
}

/*
This function can be used in 2 ways:
1: a and b are shares, then get share of ab (used in Step I of BGW)
2: b is a share and a is a constant, then share of ab (used in Step II of BGW)
*/
__m128i mulShares(__m128i a, __m128i b)
{
	return MUL(a, b);
}
/*
This function can be used in 2 ways:
1: a and b are shares, then get share of ab (used in Step I of BGW)
2: b is a share and a is a constant, then share of ab (used in Step II of BGW)
*/
void mulShares(__m128i a, __m128i b, __m128i* ab)
{
	MULT(a, b, ab[0]);
}

//creates share for BGW round
__m128i* BGWshares(__m128i shareA, __m128i shareB, int numOfParties, int deg)//int party (won't be needed, because it will be saved globally)
{
	__m128i shareAB = mulShares(shareA, shareB);
	__m128i toSplit;
	mulShares(BGWconst, shareAB, &toSplit);
	__m128i* toSendSharesAB = createShares(toSplit, numOfParties, deg);
	return toSendSharesAB;
}

__m128i BGWrecoverShare(__m128i* BGWshares, int numOfParties)
{
	__m128i multiplicationShare = BGWshares[0];
	for (int i = 1; i < numOfParties; i++)
		MULT(multiplicationShare, BGWshares[i], multiplicationShare);
	return multiplicationShare;
}





