/*
 * ndn-fw-feedback-pitsize-difference-tag.h
 *
 *  Created on: 2013年8月5日
 *      Author: wang
 */

#ifndef NDN_FW_FEEDBACK_PITSIZE_DIFFERENCE_TAG_H_
#define NDN_FW_FEEDBACK_PITSIZE_DIFFERENCE_TAG_H_

#include "ns3/tag.h"

namespace ns3 {
namespace ndn {

/**
 * @brief Packet tag that is used to track hop count for Interest-Data pairs
 */
class FwFeedbackPitsizeDifferenceTag : public Tag
{
public:
  static TypeId
  GetTypeId (void);

  /**
   * @brief Default constructor
   */
  FwFeedbackPitsizeDifferenceTag ()
    : m_pitsizedif(0)
    { };
  
  /**
   * @brief Destructor
   */
  ~FwFeedbackPitsizeDifferenceTag () { }

  /*ADD 2017/12/13****************************************************************************/
  
  /**
   * @brief Set values of feedback
   */

  void
  SetPitSizeDif(double pitsizedif);

  double
  GetPitSizeDif() const;

  /*END ADD 2017/12/13************************************************************************/


  ////////////////////////////////////////////////////////
  // from ObjectBase
  ////////////////////////////////////////////////////////
  virtual TypeId
  GetInstanceTypeId () const;

  ////////////////////////////////////////////////////////
  // from Tag
  ////////////////////////////////////////////////////////
/**/
  virtual uint32_t
  GetSerializedSize () const;

  virtual void
  Serialize (TagBuffer i) const;

  virtual void
  Deserialize (TagBuffer i);

  virtual void
  Print (std::ostream &os) const;

private:
  double m_pitsizedif;
};

} // namespace ndn
} // namespace ns3



#endif /* NDN_FW_FEEDBACK_PITSIZE_DIFFERENCE_TAG_H_ */

