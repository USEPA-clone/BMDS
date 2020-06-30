#ifdef R_COMPILATION  
#include <RcppEigen.h>
  #include <RcppGSL.h>
#else 
  #include <Eigen/Dense>
#endif

#include <gsl/gsl_randist.h>
#include "bmdStruct.h"
#include <math.h>
#include <stdio.h>

#include <string>
#include <vector>
#include <algorithm>
#include <limits>

#include "DichHillBMD_NC.h"
#include "DichMultistageBMD_NC.h"
#include "DichLogLogisticBMD_NC.h"
#include "DichLogProbitBMD_NC.h"
#include "DichWeibullBMD_NC.h"
#include "DichGammaBMD_NC.h"
#include "DichQlinearBMD_NC.h"
#include "DichLogisticBMD_NC.h"
#include "DichProbitBMD_NC.h"
#include "IDPrior.h"

#include "bmds_entry.h"
#include "bmdStruct.h"

#include "bmd_calculate.h"
#include "dichotomous_entry_code.h"
#include "mcmc_analysis.h"

void rescale(Eigen::MatrixXd *parms, 
             dich_model model, double max_dose){
  
  Eigen::MatrixXd temp = *parms; 
  Eigen::MatrixXd add   = Eigen::MatrixXd::Zero(parms->rows(),1);
  Eigen::MatrixXd scale = Eigen::MatrixXd::Ones(parms->rows(),1); 
  switch(model){
    case dich_model::d_hill:
        add(2,0)   = temp(3,0)*log(1/max_dose); 
      break; 
    case dich_model::d_gamma:
        scale(2,0) = 1/max_dose; 
      break; 
    case dich_model::d_logistic:
        scale(1,0) = 1/max_dose; 
      break; 
    case dich_model::d_loglogistic:
        add(1,0) = temp(2,0)*log(1/max_dose); 
      break; 
    case dich_model::d_logprobit:
        add(1,0) = temp(2,0)*log(1/max_dose); 
      break; 
    case dich_model::d_multistage:
        for (int i = 1; i < parms->rows(); i++){
          scale(i,0) = pow(1/max_dose, i); 
        }
      break;
    case dich_model::d_probit:
        scale(1,0) = 1/max_dose; 
      break;
    case dich_model::d_qlinear: 
        scale(1,0) = 1/max_dose; 
      break; 
    case dich_model::d_weibull:
        scale(1,0) = pow(1/max_dose,temp(1,0)); 
      break; 
  }
  *parms =   parms->array() * scale.array(); 
  *parms +=  add; 
}


void rescale_var_matrix(Eigen::MatrixXd *var, 
                        Eigen::MatrixXd parms, 
                        dich_model model, double max_dose){
  Eigen::MatrixXd tvar = *var; 
  Eigen::MatrixXd temp = parms; 
  Eigen::MatrixXd add =   Eigen::MatrixXd::Zero(parms.rows(),parms.rows()); 
  Eigen::MatrixXd scale = Eigen::MatrixXd::Identity(parms.rows(),parms.rows());
 
  switch(model){
  case dich_model::d_hill:
    scale(2,3)   = log(1/max_dose); 
    break; 
  case dich_model::d_gamma:
    scale(2,2) = 1/max_dose; 
    break; 
  case dich_model::d_logistic:
    scale(1,1) = 1/max_dose; 
    break; 
  case dich_model::d_loglogistic:
    scale(1,2) = log(1/max_dose); 
    break; 
  case dich_model::d_logprobit:
    scale(1,2) = log(1/max_dose); 
    break; 
  case dich_model::d_multistage:
    for (int i = 1; i < parms.rows(); i++){
      scale(i,0) = pow(1/max_dose,i); 
    }
    break;
  case dich_model::d_probit:
    scale(1,1) = 1/max_dose; 
    break;
  case dich_model::d_qlinear: 
    scale(1,1) = 1/max_dose; 
    break; 
  case dich_model::d_weibull:
    scale(2,1) = log(1/max_dose)*pow(1/max_dose,temp(1,0)); 
    break; 
  }
  
  *var  =   scale * (tvar) *scale.transpose();  

}
void rescale_dichotomous_model(mcmcSamples v, dich_model model,
                               double max_dose){
  //rescale the dichotomous to the origional values
  for (int i = 0; i < v.samples.cols(); i++ ){
       Eigen::MatrixXd temp = v.samples.col(i); 
       rescale(&temp, model,max_dose);  
       v.samples.col(i) = temp; 
       v.BMD(i,0) *= max_dose; 
  }
  
  rescale(&v.map_estimate,model,max_dose); 
  
}


void transfer_dichotomous_model(bmd_analysis a, dichotomous_model_result *model){
  if (model){
    model->nparms = a.COV.rows(); 
    model->max = a.MAP; 
    for (int i = 0; i< model->dist_numE; i ++){
      double temp = double(i)/double(model->dist_numE); 
      model->bmd_dist[i] = a.BMD_CDF.inv(temp);     // BMD @ probability
     // cerr << model->bmd_dist[i] << ":" << temp << endl; 
      model->bmd_dist[model->dist_numE + i] = temp; // probability 
    }
    
    for (int i = 0; i < model->nparms; i++){
      model->parms[i] = a.MAP_ESTIMATE(i,0); 
      for (int j = 0; j < model->nparms; j++){
        model->cov[i + j*model->nparms] = a.COV(i,j); 
      }
    }
  }
}


void estimate_ma_MCMC(dichotomousMA_analysis *MA,
                      dichotomous_analysis   *DA,
                      dichotomousMA_result   *res,
                      ma_MCMCfits            *ma){ 
  return ; 
}

void estimate_ma_laplace(dichotomousMA_analysis *MA,
                         dichotomous_analysis *DA ,
                         dichotomousMA_result *res){
  return ; 
}


void estimate_sm_mcmc(dichotomous_analysis *DA, 
                      dichotomous_model_result *res,
                      bmd_analysis_MCMC *mcmc,
                      bool do_a_rescale){
 
  ///////////////////////////////////
  Eigen::MatrixXd Y(DA->n,2); 
  Eigen::MatrixXd D(DA->n,1); 
  Eigen::MatrixXd prior(DA->parms,DA->prior_cols);
  for (int i = 0; i < DA->n; i++){
      Y(i,0) = DA->Y[i]; Y(i,1) = DA->n_group[i]; 
      D(i,0) = DA->doses[i]; 
  }
  
  double  max_dose = D.maxCoeff(); 
  D = (1/max_dose) * D; 
  
  for (int i = 0; i < DA->parms; i++){
      for (int j = 0; j < DA->prior_cols; j++){
        prior(i,j) = DA->prior[i + j*DA->parms]; 
      } 
  } 
  
  // copy the prior over. 
  mcmcSamples a; 
  std::vector<bool> fixedB; 
  std::vector<double> fixedV; 
  for (int i = 0; i < prior.rows(); i++){
    fixedB.push_back(false);
    fixedV.push_back(0.0); 
  }

  switch (DA->model){
    case dich_model::d_hill:
      a =  MCMC_bmd_analysis_DNC<dich_hillModelNC,IDPrior> (Y,D,prior,
                                     fixedB, fixedV, DA->degree,
                                     DA->BMR, DA->BMD_type, DA->alpha, DA->samples);
    break; 
    case dich_model::d_gamma:
      a =  MCMC_bmd_analysis_DNC<dich_gammaModelNC,IDPrior> (Y,D,prior,
                                                            fixedB, fixedV, DA->degree,
                                                            DA->BMR, DA->BMD_type, DA->alpha, DA->samples);
  
    break; 
    case dich_model::d_logistic:
      a =  MCMC_bmd_analysis_DNC<dich_logisticModelNC,IDPrior> (Y,D,prior,
                                                            fixedB, fixedV, DA->degree,
                                                            DA->BMR, DA->BMD_type, DA->alpha, DA->samples);
    break; 
    case dich_model::d_loglogistic:
      a =  MCMC_bmd_analysis_DNC<dich_loglogisticModelNC,IDPrior> (Y,D,prior,
                                                            fixedB, fixedV, DA->degree,
                                                            DA->BMR, DA->BMD_type, DA->alpha, DA->samples);
    break;
    case dich_model::d_logprobit:
      a =  MCMC_bmd_analysis_DNC<dich_logProbitModelNC,IDPrior> (Y,D,prior,
                                                            fixedB, fixedV, DA->degree,
                                                            DA->BMR, DA->BMD_type, DA->alpha, DA->samples);
    break; 
    case dich_model::d_multistage:
      a =  MCMC_bmd_analysis_DNC<dich_multistageNC,IDPrior> (Y,D,prior,
                                                            fixedB, fixedV, DA->degree,
                                                            DA->BMR, DA->BMD_type, DA->alpha, DA->samples);
    break;
    case dich_model::d_probit: 
      a =  MCMC_bmd_analysis_DNC<dich_probitModelNC,IDPrior> (Y,D,prior,
                                                            fixedB, fixedV, DA->degree,
                                                            DA->BMR, DA->BMD_type, DA->alpha, DA->samples);
    break;
    case dich_model::d_qlinear: 
      a =  MCMC_bmd_analysis_DNC<dich_qlinearModelNC,IDPrior> (Y,D,prior,
                                                            fixedB, fixedV, DA->degree,
                                                            DA->BMR, DA->BMD_type, DA->alpha, DA->samples);
    break; 
    case dich_model::d_weibull:
      a =  MCMC_bmd_analysis_DNC<dich_weibullModelNC,IDPrior> (Y,D,prior,
                                                             fixedB, fixedV, DA->degree,
                                                             DA->BMR, DA->BMD_type, DA->alpha, DA->samples);
    default: 
    break; 
  }
  
  if (do_a_rescale){
    rescale_dichotomous_model(a, DA->model, max_dose); 
  }
  
  bmd_analysis b; 
  
  b = create_bmd_analysis_from_mcmc(DA->burnin,a);
  mcmc->model = DA->model; 
  mcmc->burnin = DA->burnin; 
  mcmc->samples = DA->samples; 
  mcmc->nparms = DA->parms; 
  transfer_mcmc_output(a,mcmc); 
  res->model = DA->model; 
  transfer_dichotomous_model(b,res);
 
  return; 
}

void estimate_sm_laplace(dichotomous_analysis *DA, 
                         dichotomous_model_result *res, 
                         bool do_a_rescale){
  
  ///////////////////////////////////
  Eigen::MatrixXd Y(DA->n,2); 
  Eigen::MatrixXd D(DA->n,1); 
  Eigen::MatrixXd prior(DA->parms,DA->prior_cols);
  for (int i = 0; i < DA->n; i++){
    Y(i,0) = DA->Y[i]; Y(i,1) = DA->n_group[i]; 
    D(i,0) = DA->doses[i]; 
  }
  
  double  max_dose = D.maxCoeff(); 
  D = (1/max_dose) * D; 
  
  for (int i = 0; i < DA->parms; i++){
    for (int j = 0; j < DA->prior_cols; j++){
      prior(i,j) = DA->prior[i + j*DA->parms]; 
    } 
  }  // copy the prior over. 
  bmd_analysis a; 
  std::vector<bool> fixedB; 
  std::vector<double> fixedV; 
  for (int i = 0; i < DA->parms; i++){
    fixedB.push_back(false);
    fixedV.push_back(0.0); 
  }

  switch (DA->model){
  case dich_model::d_hill:
    a =   bmd_analysis_DNC<dich_hillModelNC,IDPrior> (Y,D,prior,
                                                      fixedB, fixedV, DA->degree,
                                                      DA->BMR, DA->BMD_type, 
                                                      DA->alpha*0.5, 0.02);
    break; 
  case dich_model::d_gamma:

    a =   bmd_analysis_DNC<dich_gammaModelNC,IDPrior> (Y,D,prior,
                                                       fixedB, fixedV, DA->degree,
                                                       DA->BMR, DA->BMD_type,
                                                       DA->alpha*0.5,0.02);

    break; 
  case dich_model::d_logistic:
    a =   bmd_analysis_DNC<dich_logisticModelNC,IDPrior> (Y,D,prior,
                                                          fixedB, fixedV, DA->degree,
                                                          DA->BMR, DA->BMD_type, 
                                                          DA->alpha*0.5, 0.02);
    break; 
  case dich_model::d_loglogistic:
    a =   bmd_analysis_DNC<dich_loglogisticModelNC,IDPrior> (Y,D,prior,
                                                            fixedB, fixedV, DA->degree,
                                                            DA->BMR, DA->BMD_type, 
                                                            DA->alpha*0.5, 0.02);
    break;
  case dich_model::d_logprobit:
    a =   bmd_analysis_DNC<dich_logProbitModelNC,IDPrior> (Y,D,prior,
                                                           fixedB, fixedV, DA->degree,
                                                           DA->BMR, DA->BMD_type, 
                                                           DA->alpha*0.5, 0.02);
    break; 
  case dich_model::d_multistage:
    a =   bmd_analysis_DNC<dich_multistageNC,IDPrior> (Y,D,prior,
                                                       fixedB, fixedV, DA->degree,
                                                       DA->BMR, DA->BMD_type, 
                                                       0.5*DA->alpha, 0.02);
    break;
  case dich_model::d_probit: 
    a =   bmd_analysis_DNC<dich_probitModelNC,IDPrior> (Y,D,prior,
                                                        fixedB, fixedV, DA->degree,
                                                        DA->BMR, DA->BMD_type, 
                                                        0.5*DA->alpha, 0.02);
    break;
  case dich_model::d_qlinear: 
    a =   bmd_analysis_DNC<dich_qlinearModelNC,IDPrior> (Y,D,prior,
                                                         fixedB, fixedV, DA->degree,
                                                         DA->BMR, DA->BMD_type, 0.5*DA->alpha,
                                                         0.02);
    break; 
  case dich_model::d_weibull:
    a =   bmd_analysis_DNC<dich_weibullModelNC,IDPrior> (Y,D,prior,
                                                         fixedB, fixedV, DA->degree,
                                                         DA->BMR, DA->BMD_type, 0.5*DA->alpha, 
                                                         0.02);
  default: 
    break; 
  }
  
  if (do_a_rescale){
      rescale(&a.MAP_ESTIMATE,DA->model,
              max_dose);
  }
  
  transfer_dichotomous_model(a,res);
  res->model = DA->model; 
  return; 
}