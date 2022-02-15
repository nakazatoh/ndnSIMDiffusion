/*
 * ndn-fw-content-copy-tag.cc
 *
 *  Created on: 2017年12月13日
 *      Author: Kota Kaihori
 */

#include "ndn-fw-feedback-rate-tag.h"

namespace ns3 {
namespace ndn {

TypeId
FwFeedbackRateTag::GetTypeId ()
{
  static TypeId tid = TypeId("ns3::ndn::FwFeedbackRateTag")
    .SetParent<Tag>()
    .AddConstructor<FwFeedbackRateTag>()
    ;
  return tid;
}

TypeId
FwFeedbackRateTag::GetInstanceTypeId () const
{
  return FwFeedbackRateTag::GetTypeId ();
}

uint32_t
FwFeedbackRateTag::GetSerializedSize () const
{
  return sizeof(double);
}

void
FwFeedbackRateTag::Serialize (TagBuffer i) const
{
  i.WriteDouble (m_rate);
}
  
void
FwFeedbackRateTag::Deserialize (TagBuffer i)
{
  m_rate = i.ReadDouble ();
}

/*ADD 2013/12/16****************************************************************************/

void
FwFeedbackRateTag::SetRate(double rate)
{
  m_rate = rate;
}
  
double
FwFeedbackRateTag::GetRate() const
{
  return m_rate;
}

/*END ADD 2013/12/16************************************************************************/

void
FwFeedbackRateTag::Print (std::ostream &os) const
{
  os << m_rate;
}

} // namespace ndn
} // namespace ns3





