/*
 * ndn-fw-feedback-tag.h
 *
 *  Created on: 2013年8月5日
 *      Author: wang
 */

#ifndef NDN_FW_FEEDBACK_RATE_TAG_H_
#define NDN_FW_FEEDBACK_RATE_TAG_H_

#include "ns3/tag.h"

namespace ns3 {
namespace ndn {

/**
 * @brief Packet tag that is used to track hop count for Interest-Data pairs
 */
class FwFeedbackRateTag : public Tag
{
public:
  static TypeId
  GetTypeId (void);

  /**
   * @brief Default constructor
   */
  FwFeedbackRateTag ()
    : m_rate(0)
    { };
  
  /**
   * @brief Destructor
   */
  ~FwFeedbackRateTag () { }

  /*ADD 2017/12/13****************************************************************************/
  
  /**
   * @brief Get values of feedback 
   */
  void
  SetRate(double rate);
  
  double
  GetRate() const;
  
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
  double m_rate;
};

} // namespace ndn
} // namespace ns3



#endif /* NDN_FW_FEEDBACK_RATE_TAG_H_ */

