/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Communication Networks Institute at TU Dortmund University
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Pascal Jörke <pascal.joerke@tu-dortmund.de>
 */

#ifndef WINNER_PLUS_PROPAGATION_LOSS_MODEL_H
#define WINNER_PLUS_PROPAGATION_LOSS_MODEL_H

#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-environment.h>

namespace ns3 {


/**
 * \ingroup propagation
 *
 * \brief this class implements the Winner Plus propagation loss model
 * 
 * this class implements the Winner Plus propagation loss model,
 * which is used to model various area types
 * and frequencies ranging from 450 MHz to 6.0 GHz. 
 * For more information about the model, please see
 * the Winner+ Final Channel Models (D5.3)
 */
class WinnerPlusPropagationLossModel : public PropagationLossModel
{

public:

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  WinnerPlusPropagationLossModel ();
  virtual ~WinnerPlusPropagationLossModel ();


  /** 
   * \param a the first mobility model
   * \param b the second mobility model
   * 
   * \return the loss in dBm for the propagation between
   * the two given mobility models
   */
  double GetLoss (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;

private:
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  WinnerPlusPropagationLossModel (const WinnerPlusPropagationLossModel &);
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  WinnerPlusPropagationLossModel & operator = (const WinnerPlusPropagationLossModel &);

  virtual double DoCalcRxPower (double txPowerDbm,
                                Ptr<MobilityModel> a,
                                Ptr<MobilityModel> b) const;
  virtual int64_t DoAssignStreams (int64_t stream);
  
  WinnerPlusEnvironmentType m_environment;  //!< Environment Scenario
  WinnerPlusLayoutType m_layout;  //!< Layout Type (Manhattan or Hexagonal). At this moment, only Hexagonal is implemented
  bool m_los;  //!< Line of Sight (LOS or NLOS)
  double m_frequency; //!< Frequency in Hz
  double m_height_bs; //!< Height of the Base Station
};

} // namespace ns3


#endif // WINNER_PLUS_PROPAGATION_LOSS_MODEL_H

