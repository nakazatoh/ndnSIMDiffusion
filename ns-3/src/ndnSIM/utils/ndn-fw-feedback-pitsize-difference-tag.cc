/*
 * ndn-fw-feedback-pitsize-difference-tag.cc
 *
 *  Created on: 2017年12月13日
 *      Author: Kota Kaihori
 */

#include "ndn-fw-feedback-pitsize-difference-tag.h"

namespace ns3 {
namespace ndn {

TypeId
FwFeedbackPitsizeDifferenceTag::GetTypeId ()
{
  static TypeId tid = TypeId("ns3::ndn::FwFeedbackPitsizeDifferenceTag")
    .SetParent<Tag>()
    .AddConstructor<FwFeedbackPitsizeDifferenceTag>()
    ;
  return tid;
}

TypeId
FwFeedbackPitsizeDifferenceTag::GetInstanceTypeId () const
{
  return FwFeedbackPitsizeDifferenceTag::GetTypeId ();
}

uint32_t
FwFeedbackPitsizeDifferenceTag::GetSerializedSize () const
{
  return sizeof(double);
}

void
FwFeedbackPitsizeDifferenceTag::Serialize (TagBuffer i) const
{
  i.WriteDouble (m_pitsizedif);
}
  
void
FwFeedbackPitsizeDifferenceTag::Deserialize (TagBuffer i)
{
  m_pitsizedif = i.ReadDouble ();
}

/*ADD 2013/12/16****************************************************************************/
void
FwFeedbackPitsizeDifferenceTag::SetPitSizeDif(double pitsizedif)
{
  m_pitsizedif = pitsizedif;
}

double
FwFeedbackPitsizeDifferenceTag::GetPitSizeDif() const
{
  return m_pitsizedif;
}

/*END ADD 2013/12/16************************************************************************/

void
FwFeedbackPitsizeDifferenceTag::Print (std::ostream &os) const
{
  os << m_pitsizedif;
}

} // namespace ndn
} // namespace ns3





