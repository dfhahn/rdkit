//
//  Copyright (C) 2004-2008 Greg Landrum and Rational Discovery LLC
//  Copyright (c) 2014, Novartis Institutes for BioMedical Research Inc.
//
//   @@ All Rights Reserved @@
//  This file is part of the RDKit.
//  The contents are covered by the terms of the BSD license
//  which is included in the file license.txt, found at the root
//  of the RDKit source tree.
//
#ifndef __RD_ALIGN_POINTS_H__
#define __RD_ALIGN_POINTS_H__

#include <Geometry/point.h>
#include <Geometry/Transform3D.h>
#include <Numerics/Vector.h>

namespace RDNumeric {
  
  namespace Alignments {
    
    //! \brief Compute an optimal alignment (minimum sum of squared distance) between
    //! two sets of points in 3D 
    /*!
      \param refPoints      A vector of pointers to the reference points
      \param probePoints    A vector of pointers to the points to be aligned to the refPoints
      \param trans          A RDGeom::Transform3D object to capture the necessary transformation
      \param weights        A vector of weights for each of the points
      \param reflect        Add reflection is true
      \param maxIterations  Maximum number of iterations

      \return The sum of squared distances between the points

      <b>Note</b> 
      This function returns the sum of squared distance (SSR) not the RMSD
      RMSD = sqrt(SSR/numPoints)
    */
    double AlignPoints(const RDGeom::Point3DConstPtrVect &refPoints, 
                       const RDGeom::Point3DConstPtrVect &probePoints, 
                       RDGeom::Transform3D &trans,
                       const DoubleVector *weights=0, bool reflect=false, 
                       unsigned int maxIterations=50);
                       
    //! \brief Calculates eigenvalues and eigenvectors of a moments of inertia tensor of mass points.
    /*!
            \param points         points that are to be used
            \param eigenVals      an array of doubles to store the eigenvalues in (= principal moments of inertia)
            \param eigenVecs      a 3x3 array of doubles to store the eigenvectors in
            \param maxIterations  Maximum number of iterations
     */
    void getMomentsOfInertia(const RDGeom::Point3DConstPtrVect &points,
    		double eigenVals[3], double eigenVecs[3][3], const DoubleVector *weights=0, unsigned int maxIterations=50);


    //! \brief Calculate alignment transform of a set of points with annotated weights to align
    //!  points to their principal moments of inertia.
    /*!
      \param points         points that are to be aligned
      \param trans          a RDGeom::Transform3D object to capture the necessary transformation
      \param eigenVals      an array of doubles to store the eigenvalues in (= principal moments of inertia)
                            defaults to NULL (nothing is stored)
      \param eigenVecs      a 3x3 array of doubles to store the eigenvectors in
                            defaults to NULL (nothing is stored)
      \param weights        a vector of weights for each of the points
      \param maxIterations  Maximum number of iterations
     */
    void getPrincAxesTransform(const RDGeom::Point3DConstPtrVect &points,
    		RDGeom::Transform3D &trans, double eigenVals[3]=NULL, double eigenVecs[3][3]=NULL,
    		const DoubleVector *weights=0,
    		unsigned int maxIterations=50);
  }
}

#endif
      
