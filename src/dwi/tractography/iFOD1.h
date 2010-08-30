/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 25/10/09.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __dwi_tractography_iFOD1_h__
#define __dwi_tractography_iFOD1_h__

#include "point.h"
#include "math/SH.h"
#include "dwi/tractography/method.h"
#include "dwi/tractography/shared.h"
#include "dwi/tractography/calibrator.h"

namespace MR {
  namespace DWI {
    namespace Tractography {

      class iFOD1 : public MethodBase {
        public:
          class Shared : public SharedBase {
            public:
              Shared (Image::Header& source, DWI::Tractography::Properties& property_set) :
                SharedBase (source, property_set),
                lmax (Math::SH::LforN (source.dim(3))), 
                max_trials (100),
                sin_max_angle (sin (max_angle)) 
            {
              properties["method"] = "iFOD1";
              properties.set (lmax, "lmax");
              properties.set (max_trials, "max_trials");
              bool precomputed = true;
              properties.set (precomputed, "sh_precomputed");
              if (precomputed) precomputer.init (lmax);
              info ("minimum radius of curvature = " + str(step_size / ( 2.0 * sin (max_angle / 2.0))) + " mm");

              //Calibrator calibrate (max_angle, 0.2);
            }

              size_t lmax, max_trials;
              value_type sin_max_angle;
              Math::SH::PrecomputedAL<value_type> precomputer;
          };

          iFOD1 (const Shared& shared) : 
            MethodBase (shared), 
            S (shared) { } 

          bool init () 
          { 
            if (!get_data ()) return (false);

            if (!S.init_dir) {
              for (size_t n = 0; n < S.max_trials; n++) {
                dir.set (rng.normal(), rng.normal(), rng.normal());
                dir.normalise();
                value_type val = FOD (dir);
                if (!isnan (val)) {
                  if (val > S.init_threshold) {
                    prev_FOD_val = val;
                    return (true);
                  }
                }
              }   
            }   
            else {
              dir = S.init_dir;
              value_type val = FOD (dir);
              if (finite (val)) { 
                if (val > S.init_threshold) {
                  prev_FOD_val = val;
                  return (true);
                }
              }
            }   

            return (false);
          }   

          bool next () 
          {
            if (!get_data ()) return (false);

            value_type max_val_actual = 0.0;
            for (int n = 0; n < 50; n++) {
              Point<value_type> new_dir = rand_dir (dir);
              value_type val = FOD (new_dir);
              if (val > max_val_actual) max_val_actual = val;
            }
            value_type max_val = MAX (prev_FOD_val, max_val_actual);
            prev_FOD_val = max_val_actual;

            if (isnan (max_val) || max_val < S.threshold) return (false);
            max_val *= 1.5;

            size_t nmax = max_val_actual > S.threshold ? 10000 : S.max_trials;
            for (size_t n = 0; n < nmax; n++) {
              Point<value_type> new_dir = rand_dir (dir);
              value_type val = FOD (new_dir);
              if (val > S.threshold) {
                if (val > max_val) info ("max_val exceeded!!! (val = " + str(val) + ", max_val = " + str (max_val) + ")");
                if (rng.uniform() < val/max_val) {
                  dir = new_dir;
                  dir.normalise();
                  pos += S.step_size * dir;
                  return (true);
                }
              }
            }

            return (false);
          }

        protected:
          const Shared& S;
          value_type prev_FOD_val;

          value_type FOD (const Point<value_type>& d) const 
          {
            return (S.precomputer ?
                S.precomputer.value (values, d) :
                Math::SH::value (values, d, S.lmax)
                );
          }

          Point<value_type> rand_dir (const Point<value_type>& d) { return (random_direction (d, S.max_angle, S.sin_max_angle)); }
      };

    }
  }
}

#endif

