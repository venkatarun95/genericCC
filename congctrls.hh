#include "ccc.hh"

class DefaultCC: public CCC
{
public:
   DefaultCC() : 
      m_issthresh( 83333 ),
      m_bSlowStart( true ),
      m_iDupACKCount( 0 ),
      m_iLastACK( 0 )
      {}

   void init()
   {
      m_issthresh = true;
      m_bSlowStart = 83333;

      _intersend_time = 0.0;
      _the_window = 2.0;

      //setACKInterval(2);
      setRTO(1000000);
   }

	virtual void onACK(const int& ack, double receiver_timestamp __attribute((unused)), double sender_timestamp __attribute((unused)))
   {
      if (ack == m_iLastACK)
      {
         if (3 == ++ m_iDupACKCount)
            DupACKAction();
         else if (m_iDupACKCount > 3)
            _the_window += 1.0;
         else
            ACKAction();
      }
      else
      {
         if (m_iDupACKCount >= 3)
            _the_window = m_issthresh;

         m_iLastACK = ack;
         m_iDupACKCount = 1;

         ACKAction();
      }
   }

   virtual void onTimeout()
   {
      //m_issthresh = getPerfInfo()->pktFlightSize / 2;
      m_issthresh = _the_window / 2;
      if (m_issthresh < 2)
         m_issthresh = 2;

      m_bSlowStart = true;
      _the_window = 2.0;
   }

protected:
   virtual void ACKAction()
   {
      if (m_bSlowStart)
      {
         _the_window += 1.0;

         if (_the_window >= m_issthresh)
            m_bSlowStart = false;
      }
      else
         _the_window += 1.0/_the_window;
   }

   virtual void DupACKAction()
   {
      m_bSlowStart = false;

      //m_issthresh = getPerfInfo()->pktFlightSize / 2;
      m_issthresh = _the_window / 2;
      if (m_issthresh < 2)
         m_issthresh = 2;

      _the_window = m_issthresh + 3;
   }

protected:
   int m_issthresh;
   bool m_bSlowStart;

   int m_iDupACKCount;
   int m_iLastACK;
};


class CUDPBlast: public CCC
{
public:
   CUDPBlast()
   {
      _intersend_time = 1000000;
      _the_window = 83333.0;
   }

public:
   void setRate(double mbps)
   {
      //_intersend_time = (m_iMSS * 8.0) / mbps;
      _intersend_time = (1500 * 8.0) / mbps;
   }
};
