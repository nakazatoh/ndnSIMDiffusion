/*
 * ndn-fw-content-copy-tag.cc
 *
 *  Created on: 2017年12月13日
 *      Author: Kota Kaihori
 */

#include "ndn-fw-feedback-pitsize-tag.h"

namespace ns3 {
namespace ndn {

TypeId
FwFeedbackPitsizeTag::GetTypeId ()
{
  static TypeId tid = TypeId("ns3::ndn::FwFeedbackPitsizeTag")
    .SetParent<Tag>()
    .AddConstructor<FwFeedbackPitsizeTag>()
    ;
  return tid;
}

TypeId
FwFeedbackPitsizeTag::GetInstanceTypeId () const
{
  return FwFeedbackPitsizeTag::GetTypeId ();
}

uint32_t
FwFeedbackPitsizeTag::GetSerializedSize () const
{
  return sizeof(double);
}

void
FwFeedbackPitsizeTag::Serialize (TagBuffer i) const
{
  i.WriteDouble (m_pitsize);
}
  
void
FwFeedbackPitsizeTag::Deserialize (TagBuffer i)
{
  m_pitsize = i.ReadDouble ();
}

/*ADD 2013/12/16****************************************************************************/
void
FwFeedbackPitsizeTag::SetPitSize(double pitsize)
{
  m_pitsize = pitsize;
}

double
FwFeedbackPitsizeTag::GetPitSize() const
{
  return m_pitsize;
}

/*END ADD 2013/12/16************************************************************************/

void
FwFeedbackPitsizeTag::Print (std::ostream &os) const
{
  os << m_pitsize;
}

} // namespace ndn
} // namespace ns3





