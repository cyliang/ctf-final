/**********************************************************************
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
************************************************************************
	(c) 2008 by Mister P and Q  < Mister.PandQ@gmail.com >
	    2010 slightly modified by StalkR for Codegate 2010 challenge #7
***********************************************************************/

#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include "e_os.h"

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/bn.h>
#include <openssl/crypto.h>
#ifndef OPENSSL_NO_ENGINE
#include <openssl/engine.h>
#endif

// This is a modified version of the genrsa.c sample that is part of the OpenSSL package
int main(int argc, char *argv[])
{
	
	BIGNUM *p_tBNExp			= NULL; 
	RSA	   *p_tRSAKey			= NULL; 
	BN_CTX *ctx					= NULL;
	BIGNUM *r0=NULL,*r1=NULL,*r2=NULL,*r3=NULL,*tmp;
	BIGNUM local_r0,local_d,local_p;
	BIGNUM *pr0,*d,*p;
	BIO	   *out;
	unsigned long f4			= RSA_F4;
	int ok						= -1;	
	// P & Q of RSA-768bits according to http://en.wikipedia.org/wiki/RSA_numbers#RSA-768
	char *p_acP = "10920751235985119653472002230123679493859432511142648983229380510281324432153859557876565179591504400449156343235310928577830570368769797990630171164502353";
	char *p_acQ = "11692911878185126338206773900975770006358209194645635593939130467951698869436356072539681820130238883252181986286517884376233971083574264554620794757267387";

	// Init OpenSSL
	CRYPTO_malloc_init(); 
	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms(); 
	ENGINE_load_builtin_engines();

	ctx=BN_CTX_new();
	
	BN_CTX_start(ctx);
	r0 = BN_CTX_get(ctx);
	r1 = BN_CTX_get(ctx);
	r2 = BN_CTX_get(ctx);
	r3 = BN_CTX_get(ctx);
	if (r3 == NULL) goto err;

	p_tBNExp = BN_new();
	p_tRSAKey = RSA_new();

	BN_set_word(p_tBNExp, f4);

	/* We need the RSA components non-NULL */
	if(!p_tRSAKey->n && ((p_tRSAKey->n=BN_new()) == NULL)) goto err;
	if(!p_tRSAKey->d && ((p_tRSAKey->d=BN_new()) == NULL)) goto err;
	if(!p_tRSAKey->e && ((p_tRSAKey->e=BN_new()) == NULL)) goto err;
	if(!p_tRSAKey->p && ((p_tRSAKey->p=BN_new()) == NULL)) goto err;
	if(!p_tRSAKey->q && ((p_tRSAKey->q=BN_new()) == NULL)) goto err;
	if(!p_tRSAKey->dmp1 && ((p_tRSAKey->dmp1=BN_new()) == NULL)) goto err;
	if(!p_tRSAKey->dmq1 && ((p_tRSAKey->dmq1=BN_new()) == NULL)) goto err;
	if(!p_tRSAKey->iqmp && ((p_tRSAKey->iqmp=BN_new()) == NULL)) goto err;

	// Copy the defauld (F4) exponent to the RSA key
	BN_copy(p_tRSAKey->e, p_tBNExp);

	// Convert the 2 decimal values for P and Q to the RSA key
	BN_dec2bn(&p_tRSAKey->p, p_acP);
	BN_dec2bn(&p_tRSAKey->q, p_acQ);

	// If P is bigger van Q, swap values
	if (BN_cmp(p_tRSAKey->p,p_tRSAKey->q) < 0)
		{
		tmp=p_tRSAKey->p;
		p_tRSAKey->p=p_tRSAKey->q;
		p_tRSAKey->q=tmp;
		}

	/* calculate n */
	if (!BN_mul(p_tRSAKey->n,p_tRSAKey->p,p_tRSAKey->q,ctx)) goto err;

	/* calculate d */
	if (!BN_sub(r1,p_tRSAKey->p,BN_value_one())) goto err;	/* p-1 */
	if (!BN_sub(r2,p_tRSAKey->q,BN_value_one())) goto err;	/* q-1 */
	if (!BN_mul(r0,r1,r2,ctx)) goto err;	/* (p-1)(q-1) */
	if (!(p_tRSAKey->flags & RSA_FLAG_NO_CONSTTIME))
		{
		  pr0 = &local_r0;
		  BN_with_flags(pr0, r0, BN_FLG_CONSTTIME);
		}
	else
	  pr0 = r0;
	if (!BN_mod_inverse(p_tRSAKey->d,p_tRSAKey->e,pr0,ctx)) goto err;	/* d */

	/* set up d for correct BN_FLG_CONSTTIME flag */
	if (!(p_tRSAKey->flags & RSA_FLAG_NO_CONSTTIME))
		{
		d = &local_d;
		BN_with_flags(d, p_tRSAKey->d, BN_FLG_CONSTTIME);
		}
	else
		d = p_tRSAKey->d;

	/* calculate d mod (p-1) */
	if (!BN_mod(p_tRSAKey->dmp1,d,r1,ctx)) goto err;

	/* calculate d mod (q-1) */
	if (!BN_mod(p_tRSAKey->dmq1,d,r2,ctx)) goto err;

	/* calculate inverse of q mod p */
	if (!(p_tRSAKey->flags & RSA_FLAG_NO_CONSTTIME))
		{
		p = &local_p;
		BN_with_flags(p, p_tRSAKey->p, BN_FLG_CONSTTIME);
		}
	else
		p = p_tRSAKey->p;
	if (!BN_mod_inverse(p_tRSAKey->iqmp,p_tRSAKey->q,p,ctx)) goto err;


	// Write the RSA structure to a PEM file
	if ((out=BIO_new(BIO_s_file())) == NULL)
		{
		printf("unable to create BIO for output\n");
		goto err;
		}
	if (BIO_write_filename(out,"private.pem") <= 0)
		{
		perror("private.pem");
		goto err;
		}

	//  Output RSA Key in PEM format (without password !)
	if (!PEM_write_bio_RSAPrivateKey(out,p_tRSAKey,NULL,NULL,0,NULL,NULL))
		goto err;
	
	// ALL OK !

	ok=1;
err:
	if (ok == -1)
		{
		printf("Actions failed\n");
		ok=0;
		}
	if (ctx != NULL)
		{
		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
		}

	return ok;
	}
