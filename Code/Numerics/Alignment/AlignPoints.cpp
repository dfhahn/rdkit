// $Id$
//
//  Copyright (C) 2004-2008 Greg Landrum and Rational Discovery LLC
//
//   @@ All Rights Reserved @@
//  This file is part of the RDKit.
//  The contents are covered by the terms of the BSD license
//  which is included in the file license.txt, found at the root
//  of the RDKit source tree.
//
#include "AlignPoints.h"
#include <RDBoost/Exceptions.h>
#include <RDGeneral/Invariant.h>
#include <Geometry/point.h>
#include <Geometry/Transform3D.h>
#include <Numerics/Vector.h>

#define TOLERANCE 1.e-6

namespace RDNumeric {
  
  namespace Alignments {

    
    RDGeom::Point3D _weightedSumOfPoints(const RDGeom::Point3DConstPtrVect &points, 
                                         const DoubleVector &weights) {
      PRECONDITION(points.size() == weights.size(), "");
      RDGeom::Point3DConstPtrVect_CI pti;
      RDGeom::Point3D tmpPt, res;
      const double *wData = weights.getData();
      unsigned int i = 0;
      for (pti = points.begin(); pti != points.end(); pti++) {
        tmpPt = (*(*pti));
        tmpPt *= wData[i];
        res += tmpPt;
        i++;
      }
      return res;
    }

    double _weightedSumOfLenSq(const RDGeom::Point3DConstPtrVect &points, 
                               const DoubleVector &weights) {
      PRECONDITION(points.size() == weights.size(), "");
      double res = 0.0;
      RDGeom::Point3DConstPtrVect_CI pti;
      const double *wData = weights.getData();
      unsigned int i = 0;
      for (pti = points.begin(); pti != points.end(); pti++) {
        res += (wData[i]*((*pti)->lengthSq()));
        i++;
      }
      return res;
    }

    double _sumOfWeights(const DoubleVector &weights) {
      const double *wData = weights.getData();
      double res = 0.0;
      for (unsigned int i = 0; i < weights.size(); i++) {
        CHECK_INVARIANT(wData[i] > 0.0, "Negative weight specified for a point");
        res += wData[i];
      }
      return res;
    }

    void _computeCovarianceMat(const RDGeom::Point3DConstPtrVect &refPoints, 
                               const RDGeom::Point3DConstPtrVect &probePoints, 
                               const DoubleVector &weights,
                               double covMat[3][3]) {
      unsigned int i, j;
      for (i = 0; i < 3; i++) {
        for (j=0; j < 3; j++) {
          covMat[i][j] = 0.0;
        }
      }
      unsigned int npt = refPoints.size();
      CHECK_INVARIANT(npt == probePoints.size(), "Number of points mismatch");
      CHECK_INVARIANT(npt == weights.size(), "Number of points and number of weights do not match");
      const double *wData = weights.getData();
      
      const RDGeom::Point3D *rpt, *ppt;
      double w;
      for (i = 0; i < npt; i++) {
        rpt = refPoints[i];
        ppt = probePoints[i];
        w = wData[i];
        
        covMat[0][0] += w*(ppt->x)*(rpt->x); 
        covMat[0][1] += w*(ppt->x)*(rpt->y);
        covMat[0][2] += w*(ppt->x)*(rpt->z); 

        covMat[1][0] += w*(ppt->y)*(rpt->x); 
        covMat[1][1] += w*(ppt->y)*(rpt->y);
        covMat[1][2] += w*(ppt->y)*(rpt->z); 

        covMat[2][0] += w*(ppt->z)*(rpt->x); 
        covMat[2][1] += w*(ppt->z)*(rpt->y);
        covMat[2][2] += w*(ppt->z)*(rpt->z); 
      }
    }

    void _convertCovMatToQuad(const double covMat[3][3],
                              const RDGeom::Point3D &rptSum, const RDGeom::Point3D &pptSum,
                              double wtsSum, double quad[4][4]) {
      double PxRx, PxRy, PxRz;
      double PyRx, PyRy, PyRz;
      double PzRx, PzRy, PzRz;
      double temp;
      
      temp = pptSum.x/wtsSum;
      PxRx = covMat[0][0] - temp*rptSum.x;
      PxRy = covMat[0][1] - temp*rptSum.y;
      PxRz = covMat[0][2] - temp*rptSum.z;

      temp = pptSum.y/wtsSum;
      PyRx = covMat[1][0] - temp*rptSum.x;
      PyRy = covMat[1][1] - temp*rptSum.y;
      PyRz = covMat[1][2] - temp*rptSum.z;

      temp = pptSum.z/wtsSum;
      PzRx = covMat[2][0] - temp*rptSum.x;
      PzRy = covMat[2][1] - temp*rptSum.y;
      PzRz = covMat[2][2] - temp*rptSum.z;

      quad[0][0] = -2.0*(PxRx + PyRy + PzRz);
      quad[1][1] = -2.0*(PxRx - PyRy - PzRz);
      quad[2][2] = -2.0*(PyRy - PzRz - PxRx);
      quad[3][3] = -2.0*(PzRz - PxRx - PyRy);

      quad[0][1] = quad[1][0] = 2.0*(PyRz - PzRy);
      quad[0][2] = quad[2][0] = 2.0*(PzRx - PxRz);
      quad[0][3] = quad[3][0] = 2.0*(PxRy - PyRx);
      quad[1][2] = quad[2][1] = -2.0*(PxRy + PyRx);
      quad[1][3] = quad[3][1] = -2.0*(PzRx + PxRz);
      quad[2][3] = quad[3][2] = -2.0*(PyRz + PzRy);
    }

    void _computeInertiaTensor(const RDGeom::Point3DConstPtrVect &points,
                               const RDGeom::Point3D &com,
                               const DoubleVector &weights,
                               double covMat[3][3]) {
      for (unsigned int i = 0; i < 3; i++) {
        for (unsigned int j=0; j < 3; j++) {
          covMat[i][j] = 0.0;
        }
      }
      unsigned int npt = points.size();
      CHECK_INVARIANT(npt == weights.size(), "Number of points and number of weights do not match");
      const double *wData = weights.getData();

      RDGeom::Point3D ppt;
      double w, x2, y2, z2;
      for (unsigned int i = 0; i < npt; i++) {
        ppt = (*points[i])-com;
        x2 = (ppt.x) * (ppt.x);
        y2 = (ppt.y) * (ppt.y);
        z2 = (ppt.z) * (ppt.z);

        w = wData[i];

        covMat[0][0] += w * ( y2 + z2 );
        covMat[1][1] += w * ( x2 + z2 );
        covMat[2][2] += w * ( x2 + y2 );

        covMat[0][1] -= w * (ppt.x) * (ppt.y);
        covMat[0][2] -= w * (ppt.x) * (ppt.z);
        covMat[1][2] -= w * (ppt.y) * (ppt.z);
      }
      covMat[1][0] = covMat[0][1];
      covMat[2][0] = covMat[0][2];
      covMat[2][1] = covMat[1][2];
    }
    
    //! Obtain the eigen vectors and eigen values 
    /*!
      \param quad        4x4 matrix of interest
      \param eigenVals   storage for eigen values
      \param eigenVecs   storage for eigen vectors
      \param maxIter     max number of iterations
      
      <b>Reference:<\b>
      This is essentailly a copy of the jacobi routine taken from the program 
      quatfit.c available from the Computational Chemistry Archives. 
      http://www.ccl.net/cca/software/SOURCES/C/quaternion-mol-fit/index.shtml
      E-mail jkl@osc.edu for details.
      It was written by:
      
      David J. Heisterberg
      The Ohio Supercomputer Center
      1224 Kinnear Rd.
      Columbus, OH  43212-1163
      (614)292-6036
      djh@osc.edu    djh@ohstpy.bitnet    ohstpy::djh
      Copyright: Ohio Supercomputer Center, David J. Heisterberg, 1990.
      The program can be copied and distributed freely, provided that
      this copyright in not removed. You may acknowledge the use of the
      program in published material as:
      David J. Heisterberg, 1990, unpublished results.

      Also see page 463 in Numerical Recipes in C (second edition)
    */

    unsigned int jacobi (double quad[4][4], double eigenVals[4], double eigenVecs[4][4], 
                unsigned int maxIter) {
      double offDiagNorm, diagNorm;
      double b, dma, q, t, c, s;
      double atemp, vtemp, dtemp;
      int i, j, k;
      unsigned int l;
      
      // initialize the eigen vector to Identity
      for (j = 0; j <= 3; j++) {
        for (i = 0; i <= 3; i++) eigenVecs[i][j] = 0.0;
        eigenVecs[j][j] = 1.0;
        eigenVals[j] = quad[j][j];
      }
      
      for (l = 0; l < maxIter; l++) {
        diagNorm = 0.0;
        offDiagNorm = 0.0;
        for (j = 0; j <= 3; j++) {
          diagNorm += fabs(eigenVals[j]);
          for (i = 0; i <= j - 1; i++) {
            offDiagNorm += fabs(quad[i][j]);
          }
        }
        if((offDiagNorm/diagNorm) <= TOLERANCE) 
          goto Exit_now;
        for (j = 1; j <= 3; j++) {
          for (i = 0; i <= j - 1; i++) {
            b = quad[i][j];
            if(fabs(b) > 0.0) {
              dma = eigenVals[j] - eigenVals[i];
              if((fabs(dma) + fabs(b)) <=  fabs(dma)) {
                t = b / dma;
              }
              else {
                q = 0.5 * dma / b;
                t = 1.0/(fabs(q) + sqrt(1.0+q*q));
                if(q < 0.0) {
                  t = -t;
                }
              }
              c = 1.0/sqrt(t * t + 1.0);
              s = t * c;
              quad[i][j] = 0.0;
              for (k = 0; k <= i-1; k++) {
                atemp = c * quad[k][i] - s * quad[k][j];
                quad[k][j] = s * quad[k][i] + c * quad[k][j];
                quad[k][i] = atemp;
              }
              for (k = i+1; k <= j-1; k++) {
                atemp = c * quad[i][k] - s * quad[k][j];
                quad[k][j] = s * quad[i][k] + c * quad[k][j];
                quad[i][k] = atemp;
              }
              for (k = j+1; k <= 3; k++) {
                atemp = c * quad[i][k] - s * quad[j][k];
                quad[j][k] = s * quad[i][k] + c * quad[j][k];
                quad[i][k] = atemp;
              }
              for (k = 0; k <= 3; k++) {
                vtemp = c * eigenVecs[k][i] - s * eigenVecs[k][j];
                eigenVecs[k][j] = s * eigenVecs[k][i] + c * eigenVecs[k][j];
                eigenVecs[k][i] = vtemp;
              }
              dtemp = c*c*eigenVals[i] + s*s*eigenVals[j] - 2.0*c*s*b;
              eigenVals[j] = s*s*eigenVals[i] + c*c*eigenVals[j] +  2.0*c*s*b;
              eigenVals[i] = dtemp;
            }  /* end if */
          } /* end for i */
        } /* end for j */
      } /* end for l */
      
    Exit_now:
      
      for (j = 0; j <= 2; j++) {
        k = j;
        dtemp = eigenVals[k];
        for (i = j+1; i <= 3; i++) {
          if(eigenVals[i] < dtemp) {
            k = i;
            dtemp = eigenVals[k];
          }
        }
        
        if(k > j) {
          eigenVals[k] = eigenVals[j];
          eigenVals[j] = dtemp;
          for (i = 0; i <= 3; i++) {
            dtemp = eigenVecs[i][k];
            eigenVecs[i][k] = eigenVecs[i][j];
            eigenVecs[i][j] = dtemp;
          }
        }
      }
      return l+1;
    }

    void reflectCovMat(double covMat[3][3]) {
      unsigned int i, j;
      for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
          covMat[i][j] = -covMat[i][j];
        }
      }
    }

    double AlignPoints(const RDGeom::Point3DConstPtrVect &refPoints, 
                       const RDGeom::Point3DConstPtrVect &probePoints,
                       RDGeom::Transform3D &trans,
                       const DoubleVector *weights, bool reflect, 
                       unsigned int maxIterations) {
      unsigned int npt = refPoints.size();
      PRECONDITION(npt == probePoints.size(), "Mismatch in number of points");
      trans.setToIdentity();
      const DoubleVector *wts;
      double wtsSum;
      bool ownWts;
      if (weights) {
        PRECONDITION(npt == weights->size(), "Mismatch in number of points");
        wts = weights;
        wtsSum = _sumOfWeights(*wts);
        ownWts=false;
      } else {
        wts = new DoubleVector(npt, 1.0);
        wtsSum = static_cast<double>(npt);
        ownWts=true;
      }
      
      RDGeom::Point3D rptSum = _weightedSumOfPoints(refPoints, *wts);
      RDGeom::Point3D pptSum = _weightedSumOfPoints(probePoints, *wts);
      
      double rptSumLenSq = _weightedSumOfLenSq(refPoints, *wts);
      double pptSumLenSq = _weightedSumOfLenSq(probePoints, *wts);

      double covMat[3][3];
      
      // compute the co-variance matrix
      _computeCovarianceMat(refPoints, probePoints, *wts, covMat);
      if(ownWts) {
        delete wts;
        wts=0;
      }
      if (reflect) {
        rptSum *= -1.0;
        reflectCovMat(covMat);
      }

      // convert the covariance matrix to a 4x4 matrix that needs to be diagonalized
      double quad[4][4];
      _convertCovMatToQuad(covMat, rptSum, pptSum, wtsSum, quad);
      
      // get the eigenVecs and eigenVals for the matrix
      double eigenVecs[4][4], eigenVals[4];
      jacobi(quad, eigenVals, eigenVecs, maxIterations);
      
      // get the quaternion
      double quater[4];
      quater[0] = eigenVecs[0][0];
      quater[1] = eigenVecs[1][0];
      quater[2] = eigenVecs[2][0];
      quater[3] = eigenVecs[3][0];
      
      trans.SetRotationFromQuaternion(quater);
      if (reflect) {
        // put the flip in the rotation matrix
        trans.Reflect();
      }
      // compute the SSR value
      double ssr = eigenVals[0] - (pptSum.lengthSq() + rptSum.lengthSq())/wtsSum 
        + rptSumLenSq + pptSumLenSq;
      
      if ((ssr < 0.0) && (fabs(ssr) < TOLERANCE)) {
        ssr = 0.0;
      }
      if (reflect) {
        rptSum *= -1.0;
      }

      // set the translation
      trans.TransformPoint(pptSum);
      RDGeom::Point3D move = rptSum;
      move -= pptSum;
      move /= wtsSum;
      trans.SetTranslation(move);
      return ssr;
    }

    void getMomentsOfInertia(const RDGeom::Point3DConstPtrVect &points, double eigenVals[3], double eigenVecs[3][3],
                             const DoubleVector *weights, unsigned int maxIterations){
      unsigned int npt = points.size();

      const DoubleVector *wts;
      if (weights) {
        PRECONDITION(npt == weights->size(), "Mismatch in number of points");
        wts = weights;
      } else {
        wts = new DoubleVector(npt, 1.0);
      }

      // determine center of mass
      RDGeom::Point3D com = _weightedSumOfPoints(points, *wts)/_sumOfWeights(*wts);

      double covMat[3][3];
      // compute the co-variance matrix
      _computeInertiaTensor(points, com, *wts, covMat);
      if(!weights){
        delete wts;
      }

#ifdef RDK_USE_EIGEN3
      Eigen::Matrix3d *cov=reinterpret_cast<Eigen::Matrix3d *>(covMat);
      Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> eigen(*cov);
      if(eigen.info() != Eigen::Success){
        throw ValueErrorException("Eigen3: eigenvalue calculation did not converge");
        return;
      }
      memcpy(eigenVals, &eigen.eigenvalues(),  sizeof(double)*3);
      memcpy(eigenVecs, &eigen.eigenvectors(), sizeof(double)*9);
#else
      // convert the covariance matrix to a 4x4 matrix that needs to be diagonalized
      double quad[4][4];
      memset(quad, 0, sizeof(double)*16);
      for ( unsigned int i =0; i < 3; ++i ){
        memcpy(quad[i], covMat[i], sizeof(double)*3);
      }

      // get the eigenVecs and eigenVals for the matrix
      double eigenVectors[4][4], eigenValues[4];
      jacobi(quad, eigenValues, eigenVectors, maxIterations);

      //return to original 3x3 matrix
      for ( unsigned int i = 0, j = 0; i < 4; ++i ){
        if ( !(RDKit::feq(eigenVectors[3][i], 1.0)) ){
          for ( unsigned int k = 0; k < 3; ++k ){
            eigenVecs[j][k] = eigenVectors[k][i];
          }
          eigenVals[j] = eigenValues[i];
          j++;
        }
      }
#endif
    }

    void getPrincAxesTransform(const RDGeom::Point3DConstPtrVect &probePoints,
                               RDGeom::Transform3D &trans, double eigenVals[3], double eigenVecs[3][3],
                               const DoubleVector *weights, unsigned int maxIterations){
      unsigned int npt = probePoints.size();

      const DoubleVector *wts;
      if (weights) {
        PRECONDITION(npt == weights->size(), "Mismatch in number of points");
        wts = weights;
      } else {
        wts = new DoubleVector(npt, 1.0);
      }
      RDGeom::Point3D origin = _weightedSumOfPoints(probePoints, *wts)/_sumOfWeights(*wts);

      double *eVals, (*eVecs)[3];
      if (eigenVals) {
        eVals = eigenVals;
      }
      else {
        eVals = new double[3];
      }
      if (eigenVecs) {
        eVecs = eigenVecs;
      }
      else {
        eVecs = new double[3][3];
      }
      getMomentsOfInertia(probePoints, eVals, eVecs, wts, maxIterations);
      if (!weights){
        delete wts;
      }

      // set affine transformation matrix
      trans.setToIdentity();
      double *data = trans.getData();

      //rotation
      for ( unsigned int i = 0; i < 3; i++ ){
        memcpy(data+4*i, &(eVecs[i]), sizeof(double)*3);
      }

      //translation
      origin *= -1;
      trans.TransformPoint(origin);
      trans.SetTranslation(origin);

      if (!eigenVals) {
        delete eVals;
      }
      if (!eigenVecs) {
        delete eVecs;
      }
    }
  }
}

      
