// Copyright 2018 Andreas Singraber (University of Vienna)
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <mpi.h>
#include <string.h>
#include "pair_nnp_develop.h"
#include "atom.h"
#include "comm.h"
#include "neighbor.h"
#include "neigh_list.h"
#include "neigh_request.h"
#include "memory.h"
#include "error.h"
#include "update.h"
#include "utils.h"

using namespace LAMMPS_NS;


/* ---------------------------------------------------------------------- */

PairNNPDevelop::PairNNPDevelop(LAMMPS *lmp) : PairNNP(lmp) {}

/* ---------------------------------------------------------------------- */

void PairNNPDevelop::compute(int eflag, int vflag)
{
  if(eflag || vflag) ev_setup(eflag,vflag);
  else evflag = vflag_fdotr = eflag_global = eflag_atom = 0;

  // Set number of local atoms and add index and element.
  interface.setLocalAtoms(atom->nlocal,atom->tag,atom->type);

  // Transfer local neighbor list to NNP interface.
  transferNeighborList();

  // Compute symmetry functions, atomic neural networks and add up energy.
  interface.process();

  // Do all stuff related to extrapolation warnings.
  if(showew == true || showewsum > 0 || maxew >= 0) {
    handleExtrapolationWarnings();
  }

  // Calculate forces of local and ghost atoms.
  interface.getForces(atom->f);

  // Add energy contribution to total energy.
  if (eflag_global)
     ev_tally(0,0,atom->nlocal,1,interface.getEnergy(),0.0,0.0,0.0,0.0,0.0);

  // Add atomic energy if requested (CAUTION: no physical meaning!).
  if (eflag_atom)
    for (int i = 0; i < atom->nlocal; ++i)
      eatom[i] = interface.getAtomicEnergy(i);

  // If virial needed calculate via F dot r.
  if (vflag_fdotr) virial_fdotr_compute();
}