/*
Copyright (c) 2009-2012, Structural Bioinformatics Laboratory, Boston University
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
- Neither the name of the author nor the names of its contributors may be used
  to endorse or promote products derived from this software without specific
  prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>

#include _MOL_INCLUDE_

float pairwise_potential_energy(struct atomgrp *agA, struct atomgrp *agB,
				struct prm *prm, int only_sab)
{
	int Aatomi, Batomi;	// loop iters

	// squared vals for euclidean dist
	float r1sq = _mol_sq(prm->pwpot->r1);
	float r2sq = _mol_sq(prm->pwpot->r2);

	float E = 0;

	if (prm->pwpot->r1 < 0.0 || prm->pwpot->r2 < 0.0) {
		fprintf(stderr, "begin error\n");
		fprintf(stderr,
			"at least one of the potential's bin limits is less than 0\n");
		fprintf(stderr, "end error\n");
		exit(EXIT_FAILURE);
	}
	// loop through every atom in agA
	for (Aatomi = 0; Aatomi < agA->natoms; Aatomi++) {
		float AX;
		float AY;
		float AZ;
		int subA;
		int Atypen;
		if (only_sab && !agA->atoms[Aatomi].sa)
			continue;

		Atypen = agA->atoms[Aatomi].atom_typen;
		if (Atypen < 0 || Atypen > prm->natoms - 1) {
			fprintf(stderr, "begin error\n");
			fprintf(stderr,
				"atom type number of atom index %d is not defined in the argument atom prm\n",
				Aatomi);
			fprintf(stderr, "end error\n");
			exit(EXIT_FAILURE);
		}

		subA = prm->atoms[Atypen].subid;	// get subatom mapping
		if (subA < 0)
			continue;	// ignore this subatom type
		if (subA > prm->nsubatoms - 1) {
			fprintf(stderr, "begin error\n");
			fprintf(stderr,
				"the argument atom prm subatom mapping of atom %d",
				Atypen);
			fprintf(stderr,
				"is greater than the maximum subatom type index\n");
			fprintf(stderr, "end error\n");
			exit(EXIT_FAILURE);
		}

		AX = agA->atoms[Aatomi].X;
		AY = agA->atoms[Aatomi].Y;
		AZ = agA->atoms[Aatomi].Z;

		// loop through every atom in agB
		for (Batomi = 0; Batomi < agB->natoms; Batomi++) {
			int Btypen;
			int subB;
			float BX;
			float BY;
			float BZ;
			float rsq;
			if (only_sab && !agB->atoms[Batomi].sa)
				continue;

			Btypen = agB->atoms[Batomi].atom_typen;
			if (Atypen < 0 || Atypen > prm->natoms - 1) {
				fprintf(stderr, "begin error\n");
				fprintf(stderr,
					"atom type number of atom index %d is not defined in the argument atom prm\n",
					Batomi);
				fprintf(stderr, "end error\n");
				exit(EXIT_FAILURE);
			}

			subB = prm->atoms[Btypen].subid;	// get subatom mapping
			if (subB < 0)
				continue;	// ignore this subatom type
			if (subB > prm->nsubatoms - 1) {
				fprintf(stderr, "begin error\n");
				fprintf(stderr,
					"the argument atom prm subatom mapping of atom %d",
					Btypen);
				fprintf(stderr,
					"is greater than the maximum subatom type index\n");
				fprintf(stderr, "end error\n");
				exit(EXIT_FAILURE);
			}

			BX = agB->atoms[Batomi].X;
			BY = agB->atoms[Batomi].Y;
			BZ = agB->atoms[Batomi].Z;

			// calculate euclidean distance
			rsq = (_mol_sq(AX - BX) +
			       _mol_sq(AY - BY) + _mol_sq(AZ - BZ));

			if (rsq >= r1sq && rsq < r2sq)	// atom distance is within the bin
			{
				int ek;
				for (ek = 0; ek < prm->pwpot->k; ek++) {
					E += prm->pwpot->lambdas[ek] *
					    prm->pwpot->
					    Xs[(ek * prm->nsubatoms) +
					       subA] * prm->pwpot->Xs[(ek *
								       prm->
								       nsubatoms)
								      + subB];
				}
			}
		}
	}

	return E;
}

float coulombic_elec_energy(struct atomgrp *agA, struct atomgrp *agB,
			    struct prm *prm)
{
	int Aatomi, Batomi;	// loop iters

	float E = 0;

	float maxr = 20.0;
	float maxrsq = maxr * maxr;
	float minr = 2.0;
	float minrsq = minr * minr;

	// loop through every atom in agA
	for (Aatomi = 0; Aatomi < agA->natoms; Aatomi++) {
		int Atypen = agA->atoms[Aatomi].atom_typen;
		float q1;
		float AX;
		float AY;
		float AZ;
		if (Atypen < 0 || Atypen > prm->natoms - 1) {
			fprintf(stderr,
				"atom type number of atom index %d is not defined in atom prm\n",
				Aatomi);
			exit(EXIT_FAILURE);
		}

		q1 = prm->atoms[Atypen].q;

		AX = agA->atoms[Aatomi].X;
		AY = agA->atoms[Aatomi].Y;
		AZ = agA->atoms[Aatomi].Z;

		// loop through every atom in agB
		for (Batomi = 0; Batomi < agB->natoms; Batomi++) {
			int Btypen = agB->atoms[Batomi].atom_typen;
			float q2;
			float BX;
			float BY;
			float BZ;
			float rsq;
			if (Btypen < 0 || Btypen > prm->natoms - 1) {
				fprintf(stderr,
					"atom type number of atom index %d is not defined in the atom prm\n",
					Batomi);
				exit(EXIT_FAILURE);
			}

			q2 = prm->atoms[Btypen].q;

			BX = agB->atoms[Batomi].X;
			BY = agB->atoms[Batomi].Y;
			BZ = agB->atoms[Batomi].Z;

			// calculate euclidean distance
			rsq = (_mol_sq(AX - BX) +
			       _mol_sq(AY - BY) + _mol_sq(AZ - BZ));

			//E += (q1 * q2) / rsq;
			if (rsq <= maxrsq) {
				if (rsq < minrsq)	// set a limit
					rsq = minrsq;

				E += (q1 * q2) * ((1 / rsq) -
						  ((1 / maxrsq) *
						   (2 - (rsq / maxrsq))));
			}
		}
	}

	return E;
}

float nummod_energy(struct atomgrp *agA, struct atomgrp *agB, struct prm *prm)
{
	int Aatomi, Batomi;	// loop iters

	float vrE = 0;
	float vaE = 0;
	float eE = 0;
	float E;

	float r2sq = _mol_sq(6.5);

	// loop through every atom in agA
	for (Aatomi = 0; Aatomi < agA->natoms; Aatomi++) {
		int Atypen = agA->atoms[Aatomi].atom_typen;
		float r1sq;
		float q1;
		float AX;
		float AY;
		float AZ;
		if (Atypen < 0 || Atypen > prm->natoms - 1) {
			fprintf(stderr,
				"atom type number of atom index %d is not defined in atom prm\n",
				Aatomi);
			exit(EXIT_FAILURE);
		}

		r1sq = _mol_sq(prm->atoms[Atypen].r);
		q1 = prm->atoms[Atypen].q;

		AX = agA->atoms[Aatomi].X;
		AY = agA->atoms[Aatomi].Y;
		AZ = agA->atoms[Aatomi].Z;

		// loop through every atom in agB
		for (Batomi = 0; Batomi < agB->natoms; Batomi++) {
			int Btypen = agB->atoms[Batomi].atom_typen;
			float q2;
			float BX;
			float BY;
			float BZ;
			float rsq;
			if (Btypen < 0 || Btypen > prm->natoms - 1) {
				fprintf(stderr,
					"atom type number of atom index %d is not defined in the atom prm\n",
					Batomi);
				exit(EXIT_FAILURE);
			}
			//float r2 = prm->atoms[Btypen].r;
			q2 = prm->atoms[Btypen].q;

			BX = agB->atoms[Batomi].X;
			BY = agB->atoms[Batomi].Y;
			BZ = agB->atoms[Batomi].Z;

			// calculate euclidean distance
			rsq = (_mol_sq(AX - BX) +
			       _mol_sq(AY - BY) + _mol_sq(AZ - BZ));

			if (rsq < r1sq) {
				//vrE += vw * 1.0;
				vrE += 1.0;
			} else if (rsq < r2sq) {
				//vaE -= vw * 1.0;
				vaE -= 1.0;
			}
			//E += ew * ((q1 * q2) / rsq);
			eE += ((q1 * q2) / rsq);
		}
	}

	printf("vrE: %.3f\n", vrE);
	printf("vaE: %.3f\n", vaE);
	printf("eE: %.3f\n", eE);

	E = 0.0;

	return E;
}

void test_energy_grads(struct atomgrp *ag, void *minprms,
		       void (*egfun) (int, double *, void *, double *,
				      double *), double delta)
{
	double en_before;
	double *numerical_grads = _mol_calloc(ag->natoms * 3, sizeof(double));

	egfun(0, NULL, minprms, &en_before, NULL);

	for (int i = 0; i < ag->natoms; i++) {
		double en_after;
		double temp = ag->atoms[i].X;
		ag->atoms[i].X += delta;
		egfun(0, NULL, minprms, &en_after, NULL);
		numerical_grads[i * 3] = (en_after - en_before) / delta;
		ag->atoms[i].X = temp;

		temp = ag->atoms[i].Y;
		ag->atoms[i].Y += delta;
		egfun(0, NULL, minprms, &en_after, NULL);
		numerical_grads[i * 3 + 1] = (en_after - en_before) / delta;
		ag->atoms[i].Y = temp;

		temp = ag->atoms[i].Z;
		ag->atoms[i].Z += delta;
		egfun(0, NULL, minprms, &en_after, NULL);
		numerical_grads[i * 3 + 2] = (en_after - en_before) / delta;
		ag->atoms[i].Z = temp;
	}

	egfun(0, NULL, minprms, &en_before, NULL);

	for (int i = 0; i < ag->natoms; i++) {
		printf("Analytical gradient %d: %.12f %.12f %.12f\n", i,
		       ag->atoms[i].GX, ag->atoms[i].GY, ag->atoms[i].GZ);
		printf("Numerical  gradient %d: %.12f %.12f %.12f\n", i,
		       numerical_grads[i * 3], numerical_grads[i * 3 + 1],
		       numerical_grads[i * 3 + 2]);
		printf("Difference in gradient %d: %.12g %.12g %.12g\n", i,
		       ag->atoms[i].GX + numerical_grads[i * 3],
		       ag->atoms[i].GY + numerical_grads[i * 3 + 1],
		       ag->atoms[i].GZ + numerical_grads[i * 3 + 2]);
	}
}
