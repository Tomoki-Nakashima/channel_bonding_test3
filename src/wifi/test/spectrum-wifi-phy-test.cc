/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Washington
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
 */

#include <algorithm>
#include "ns3/test.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/wifi-spectrum-value-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-spectrum-signal-parameters.h"
#include "ns3/wifi-phy-listener.h"
#include "ns3/log.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-ppdu.h"
#include "ns3/wifi-utils.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SpectrumWifiPhyBasicTest");

static const uint8_t CHANNEL_NUMBER = 36;
static const uint32_t FREQUENCY = 5180; // MHz
static const uint16_t CHANNEL_WIDTH = 20; // MHz
static const uint16_t GUARD_WIDTH = CHANNEL_WIDTH; // MHz (expanded to channel width to model spectrum mask)

class ExtSpectrumWifiPhy : public SpectrumWifiPhy
{
public:
  using SpectrumWifiPhy::SpectrumWifiPhy;
  using SpectrumWifiPhy::GetBand;
};

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Spectrum Wifi Phy Basic Test
 */
class SpectrumWifiPhyBasicTest : public TestCase
{
public:
  SpectrumWifiPhyBasicTest ();
  /**
   * Constructor
   *
   * \param name reference name
   */
  SpectrumWifiPhyBasicTest (std::string name);
  virtual ~SpectrumWifiPhyBasicTest ();

protected:
  virtual void DoSetup (void);
  Ptr<SpectrumWifiPhy> m_phy; ///< Phy
  /**
   * Make signal function
   * \param txPowerWatts the transmit power in watts
   * \returns Ptr<SpectrumSignalParameters>
   */
  Ptr<SpectrumSignalParameters> MakeSignal (double txPowerWatts);
  /**
   * Send signal function
   * \param txPowerWatts the transmit power in watts
   */
  void SendSignal (double txPowerWatts);
  /**
   * Spectrum wifi receive success function
   * \param psdu the PSDU
   * \param rxSignalInfo the info on the received signal (\see RxSignalInfo)
   * \param txVector the transmit vector
   * \param statusPerMpdu reception status per MPDU
   */
  void SpectrumWifiPhyRxSuccess (Ptr<WifiPsdu> psdu, RxSignalInfo rxSignalInfo,
                                 WifiTxVector txVector, std::vector<bool> statusPerMpdu);
  /**
   * Spectrum wifi receive failure function
   * \param psdu the PSDU
-   */
  void SpectrumWifiPhyRxFailure (Ptr<WifiPsdu> psdu);
  uint32_t m_count; ///< count

private:
  virtual void DoRun (void);

  uint64_t m_uid;
};

SpectrumWifiPhyBasicTest::SpectrumWifiPhyBasicTest ()
  : TestCase ("SpectrumWifiPhy test case receives one packet"),
    m_count (0)
{
}

SpectrumWifiPhyBasicTest::SpectrumWifiPhyBasicTest (std::string name)
  : TestCase (name),
    m_count (0),
    m_uid (0)
{
}

// Make a Wi-Fi signal to inject directly to the StartRx() method
Ptr<SpectrumSignalParameters>
SpectrumWifiPhyBasicTest::MakeSignal (double txPowerWatts)
{
  WifiTxVector txVector = WifiTxVector (WifiPhy::GetOfdmRate6Mbps (), 0, WIFI_PREAMBLE_LONG, 800, 1, 1, 0, 20, false, false);

  Ptr<Packet> pkt = Create<Packet> (1000);
  WifiMacHeader hdr;

  hdr.SetType (WIFI_MAC_QOSDATA);
  hdr.SetQosTid (0);

  Ptr<WifiPsdu> psdu = Create<WifiPsdu> (pkt, hdr);
  Time txDuration = m_phy->CalculateTxDuration (psdu->GetSize (), txVector, m_phy->GetFrequency ());

  Ptr<WifiPpdu> ppdu = Create<WifiPpdu> (psdu, txVector, txDuration, FREQUENCY, m_uid++);

  Ptr<SpectrumValue> txPowerSpectrum = WifiSpectrumValueHelper::CreateOfdmTxPowerSpectralDensity (FREQUENCY, CHANNEL_WIDTH, txPowerWatts, GUARD_WIDTH);
  Ptr<WifiSpectrumSignalParameters> txParams = Create<WifiSpectrumSignalParameters> ();
  txParams->psd = txPowerSpectrum;
  txParams->txPhy = 0;
  txParams->duration = txDuration;
  txParams->ppdu = ppdu;

  return txParams;
}

// Make a Wi-Fi signal to inject directly to the StartRx() method
void
SpectrumWifiPhyBasicTest::SendSignal (double txPowerWatts)
{
  m_phy->StartRx (MakeSignal (txPowerWatts));
}

void
SpectrumWifiPhyBasicTest::SpectrumWifiPhyRxSuccess (Ptr<WifiPsdu> psdu, RxSignalInfo rxSignalInfo,
                                                    WifiTxVector txVector, std::vector<bool> statusPerMpdu)
{
  NS_LOG_FUNCTION (this << *psdu << rxSignalInfo << txVector);
  m_count++;
}

void
SpectrumWifiPhyBasicTest::SpectrumWifiPhyRxFailure (Ptr<WifiPsdu> psdu)
{
  NS_LOG_FUNCTION (this << *psdu);
  m_count++;
}

SpectrumWifiPhyBasicTest::~SpectrumWifiPhyBasicTest ()
{
}

// Create necessary objects, and inject signals.  Test that the expected
// number of packet receptions occur.
void
SpectrumWifiPhyBasicTest::DoSetup (void)
{
  m_phy = CreateObject<SpectrumWifiPhy> ();
  m_phy->ConfigureStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
  Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel> ();
  m_phy->SetErrorRateModel (error);
  m_phy->SetChannelNumber (CHANNEL_NUMBER);
  m_phy->SetFrequency (FREQUENCY);
  m_phy->SetReceiveOkCallback (MakeCallback (&SpectrumWifiPhyBasicTest::SpectrumWifiPhyRxSuccess, this));
  m_phy->SetReceiveErrorCallback (MakeCallback (&SpectrumWifiPhyBasicTest::SpectrumWifiPhyRxFailure, this));
}

// Test that the expected number of packet receptions occur.
void
SpectrumWifiPhyBasicTest::DoRun (void)
{
  double txPowerWatts = 0.010;
  // Send packets spaced 1 second apart; all should be received
  Simulator::Schedule (Seconds (1), &SpectrumWifiPhyBasicTest::SendSignal, this, txPowerWatts);
  Simulator::Schedule (Seconds (2), &SpectrumWifiPhyBasicTest::SendSignal, this, txPowerWatts);
  Simulator::Schedule (Seconds (3), &SpectrumWifiPhyBasicTest::SendSignal, this, txPowerWatts);
  // Send packets spaced 1 microsecond second apart; none should be received (PHY header reception failure)
  Simulator::Schedule (MicroSeconds (4000000), &SpectrumWifiPhyBasicTest::SendSignal, this, txPowerWatts);
  Simulator::Schedule (MicroSeconds (4000001), &SpectrumWifiPhyBasicTest::SendSignal, this, txPowerWatts);
  Simulator::Run ();
  Simulator::Destroy ();

  NS_TEST_ASSERT_MSG_EQ (m_count, 3, "Didn't receive right number of packets");
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test Phy Listener
 */
class TestPhyListener : public ns3::WifiPhyListener
{
public:
  /**
   * Create a test PhyListener
   *
   */
  TestPhyListener (void)
    : m_notifyRxStart (0),
      m_notifyRxEndOk (0),
      m_notifyRxEndError (0),
      m_notifyMaybeCcaBusyStart (0)
  {
  }
  virtual ~TestPhyListener ()
  {
  }
  virtual void NotifyRxStart (Time duration)
  {
    NS_LOG_FUNCTION (this << duration);
    ++m_notifyRxStart;
  }
  virtual void NotifyRxEndOk (void)
  {
    NS_LOG_FUNCTION (this);
    ++m_notifyRxEndOk;
  }
  virtual void NotifyRxEndError (void)
  {
    NS_LOG_FUNCTION (this);
    ++m_notifyRxEndError;
  }
  virtual void NotifyTxStart (Time duration, double txPowerDbm)
  {
    NS_LOG_FUNCTION (this << duration << txPowerDbm);
  }
  virtual void NotifyMaybeCcaBusyStart (Time duration)
  {
    NS_LOG_FUNCTION (this);
    ++m_notifyMaybeCcaBusyStart;
  }
  virtual void NotifySwitchingStart (Time duration)
  {
  }
  virtual void NotifySleep (void)
  {
  }
  virtual void NotifyOff (void)
  {
  }
  virtual void NotifyWakeup (void)
  {
  }
  virtual void NotifyOn (void)
  {
  }
  uint32_t m_notifyRxStart; ///< notify receive start
  uint32_t m_notifyRxEndOk; ///< notify receive end OK
  uint32_t m_notifyRxEndError; ///< notify receive end error
  uint32_t m_notifyMaybeCcaBusyStart; ///< notify maybe CCA busy start
private:
};

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Spectrum Wifi Phy Listener Test
 */
class SpectrumWifiPhyListenerTest : public SpectrumWifiPhyBasicTest
{
public:
  SpectrumWifiPhyListenerTest ();
  virtual ~SpectrumWifiPhyListenerTest ();
private:
  virtual void DoSetup (void);
  virtual void DoRun (void);
  TestPhyListener* m_listener; ///< listener
};

SpectrumWifiPhyListenerTest::SpectrumWifiPhyListenerTest ()
  : SpectrumWifiPhyBasicTest ("SpectrumWifiPhy test operation of WifiPhyListener")
{
}

SpectrumWifiPhyListenerTest::~SpectrumWifiPhyListenerTest ()
{
}

void
SpectrumWifiPhyListenerTest::DoSetup (void)
{
  SpectrumWifiPhyBasicTest::DoSetup ();
  m_listener = new TestPhyListener;
  m_phy->RegisterListener (m_listener);
}

void
SpectrumWifiPhyListenerTest::DoRun (void)
{
  double txPowerWatts = 0.010;
  Simulator::Schedule (Seconds (1), &SpectrumWifiPhyListenerTest::SendSignal, this, txPowerWatts);
  Simulator::Run ();

  NS_TEST_ASSERT_MSG_EQ (m_count, 1, "Didn't receive right number of packets");
  NS_TEST_ASSERT_MSG_EQ (m_listener->m_notifyRxStart, 1, "Didn't receive NotifyRxStart");
  NS_TEST_ASSERT_MSG_EQ (m_listener->m_notifyRxEndOk, 1, "Didn't receive NotifyRxEnd");
  NS_TEST_ASSERT_MSG_EQ (m_listener->m_notifyMaybeCcaBusyStart, 0, "Received NotifyMaybeCcaBusyStart unexpectedly");

  Simulator::Destroy ();
  delete m_listener;
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Spectrum Wifi Phy Filter Test
 */
class SpectrumWifiPhyFilterTest : public TestCase
{
public:
  SpectrumWifiPhyFilterTest ();
  /**
   * Constructor
   *
   * \param name reference name
   */
  SpectrumWifiPhyFilterTest (std::string name);
  virtual ~SpectrumWifiPhyFilterTest ();

private:
  virtual void DoSetup (void);
  virtual void DoRun (void);

  /**
   * Run one function
   */
  void RunOne ();

  /**
   * Send PPDU function
   */
  void SendPpdu (void);

  /**
   * Callback triggered when a packet is received by the PHYs
   * \param context the context
   * \param p the received packet
   * \param rxPowersW the received power per channel band in watts
   */
  void RxCallback (Ptr<const Packet> p, RxPowerWattPerChannelBand rxPowersW);

  Ptr<ExtSpectrumWifiPhy> m_txPhy; ///< TX PHY
  Ptr<ExtSpectrumWifiPhy> m_rxPhy; ///< RX PHY

  uint16_t m_txChannelWidth; ///< TX channel width (MHz)
  uint16_t m_rxChannelWidth; ///< RX channel width (MHz)
};

SpectrumWifiPhyFilterTest::SpectrumWifiPhyFilterTest ()
  : TestCase ("SpectrumWifiPhy test RX filters"),
    m_txChannelWidth (20),
    m_rxChannelWidth (20)
{
}

SpectrumWifiPhyFilterTest::SpectrumWifiPhyFilterTest (std::string name)
  : TestCase (name)
{
}

void
SpectrumWifiPhyFilterTest::SendPpdu (void)
{
  WifiTxVector txVector = WifiTxVector (WifiPhy::GetHeMcs0 (), 0, WIFI_PREAMBLE_HE_SU, 800, 1, 1, 0, m_txChannelWidth, false, false);
  Ptr<Packet> pkt = Create<Packet> (1000);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_QOSDATA);
  hdr.SetQosTid (0);
  hdr.SetAddr1 (Mac48Address ("00:00:00:00:00:01"));
  hdr.SetSequenceNumber (1);
  Ptr<WifiPsdu> psdu = Create<WifiPsdu> (pkt, hdr);
  m_txPhy->Send (WifiPsduMap ({std::make_pair (SU_STA_ID, psdu)}), txVector);
}

SpectrumWifiPhyFilterTest::~SpectrumWifiPhyFilterTest ()
{
  m_txPhy = 0;
  m_rxPhy = 0;
}

void
SpectrumWifiPhyFilterTest::RxCallback (Ptr<const Packet> p, RxPowerWattPerChannelBand rxPowersW)
{
  for (auto const& pair : rxPowersW)
    {
      NS_LOG_INFO ("band: (" << pair.first.first << ";" << pair.first.second << ") -> powerW=" << pair.second << " (" << WToDbm (pair.second) << " dBm)");
    }

  size_t numBands = rxPowersW.size ();
  size_t expectedNumBands = std::max (1, (m_rxChannelWidth / 20));
  if (m_rxChannelWidth == 20)
    {
      expectedNumBands += 9; /* RU_26_TONE */
      expectedNumBands += 4; /* RU_52_TONE */
      expectedNumBands += 2; /* RU_106_TONE */
      expectedNumBands += 1; /* RU_242_TONE */
    }
  else if (m_rxChannelWidth == 40)
    {
      expectedNumBands += 18; /* RU_26_TONE */
      expectedNumBands += 8; /* RU_52_TONE */
      expectedNumBands += 4; /* RU_106_TONE */
      expectedNumBands += 2; /* RU_242_TONE */
      expectedNumBands += 1; /* RU_484_TONE */
    }
  else if (m_rxChannelWidth >= 80)
    {
      expectedNumBands += 37 * (m_rxChannelWidth / 80); /* RU_26_TONE */
      expectedNumBands += 16 * (m_rxChannelWidth / 80); /* RU_52_TONE */
      expectedNumBands += 8 * (m_rxChannelWidth / 80); /* RU_106_TONE */
      expectedNumBands += 4 * (m_rxChannelWidth / 80); /* RU_242_TONE */
      expectedNumBands += 2 * (m_rxChannelWidth / 80); /* RU_484_TONE */
      expectedNumBands += 1 * (m_rxChannelWidth / 80); /* RU_996_TONE */
      if (m_rxChannelWidth == 160)
        {
          ++expectedNumBands; /* RU_2x996_TONE */
        }
    }
  NS_TEST_ASSERT_MSG_EQ (numBands, expectedNumBands, "Total number of bands handled by the receiver is incorrect");

  uint16_t channelWidth = std::min (m_txChannelWidth, m_rxChannelWidth);
  double totalPowerW = 0.0;
  WifiSpectrumBand band;
  if (channelWidth <= 20)
    {
      band = m_rxPhy->GetBand (channelWidth);
      auto it = std::find_if (rxPowersW.begin (), rxPowersW.end(),
          [&band](const std::pair<WifiSpectrumBand, double>& element){ return element.first == band; } );
      totalPowerW = it->second;
    }
  else
    {
      for (uint8_t i = 0; i < (channelWidth / 20); i++)
        {
          band = m_rxPhy->GetBand (20, i);
          auto it = std::find_if (rxPowersW.begin (), rxPowersW.end(),
              [&band](const std::pair<WifiSpectrumBand, double>& element){ return element.first == band; } );
          totalPowerW += it->second;
        }
    }
  NS_LOG_INFO ("powerW total: " << totalPowerW << " (" << WToDbm (totalPowerW) << " dBm)");
  int totalRxPower = static_cast<int> (WToDbm (totalPowerW) + 0.5);
  int expectedTotalRxPower;
  if (m_txChannelWidth <= m_rxChannelWidth)
    {
      //PHY sends at 16 dBm, and since there is no loss, this should be the total power at the receiver.
      expectedTotalRxPower = 16;
    }
  else
    {
      //Only a part of the transmitted power is received
      expectedTotalRxPower = 16 - static_cast<int> (RatioToDb (m_txChannelWidth / m_rxChannelWidth));
    }
  NS_TEST_ASSERT_MSG_EQ (totalRxPower, expectedTotalRxPower, "Total received power is not correct");
  if ((m_txChannelWidth <= m_rxChannelWidth) && (channelWidth >= 20))
    {
      band = m_rxPhy->GetBand (20, 0); //primary 20 MHz
      auto it = std::find_if (rxPowersW.begin (), rxPowersW.end(),
          [&band](const std::pair<WifiSpectrumBand, double>& element){ return element.first == band; } );
      NS_LOG_INFO ("powerW in primary 20 MHz channel: " << it->second << " (" << WToDbm (it->second) << " dBm)");
      int rxPowerPrimaryChannel20 = static_cast<int> (WToDbm (it->second) + 0.5);
      int expectedRxPowerPrimaryChannel20 = 16 - static_cast<int> (RatioToDb (channelWidth / 20));
      NS_ASSERT (rxPowerPrimaryChannel20 == expectedRxPowerPrimaryChannel20);
      NS_TEST_ASSERT_MSG_EQ (rxPowerPrimaryChannel20, expectedRxPowerPrimaryChannel20, "Received power in the primary 20 MHz band is not correct");
    }
}

void
SpectrumWifiPhyFilterTest::DoSetup (void)
{
  //WifiHelper::EnableLogComponents ();
  //LogComponentEnable ("SpectrumWifiPhyBasicTest", LOG_LEVEL_ALL);

  Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel> ();
  Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel> ();
  lossModel->SetFrequency (5.180e9);
  spectrumChannel->AddPropagationLossModel (lossModel);
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
  spectrumChannel->SetPropagationDelayModel (delayModel);
  
  Ptr<Node> txNode = CreateObject<Node> ();
  Ptr<WifiNetDevice> txDev = CreateObject<WifiNetDevice> ();
  m_txPhy = CreateObject<ExtSpectrumWifiPhy> ();
  m_txPhy->CreateWifiSpectrumPhyInterface (txDev);
  m_txPhy->ConfigureStandard (WIFI_PHY_STANDARD_80211ax_5GHZ);
  Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel> ();
  m_txPhy->SetErrorRateModel (error);
  m_txPhy->SetDevice (txDev);
  m_txPhy->SetChannel (spectrumChannel);
  Ptr<ConstantPositionMobilityModel> apMobility = CreateObject<ConstantPositionMobilityModel> ();
  m_txPhy->SetMobility (apMobility);
  txDev->SetPhy (m_txPhy);
  txNode->AggregateObject (apMobility);
  txNode->AddDevice (txDev);

  Ptr<Node> rxNode = CreateObject<Node> ();
  Ptr<WifiNetDevice> rxDev = CreateObject<WifiNetDevice> ();
  m_rxPhy = CreateObject<ExtSpectrumWifiPhy> ();
  m_rxPhy->CreateWifiSpectrumPhyInterface (rxDev);
  m_rxPhy->ConfigureStandard (WIFI_PHY_STANDARD_80211ax_5GHZ);
  m_rxPhy->SetErrorRateModel (error);
  m_rxPhy->SetChannel (spectrumChannel);
  Ptr<ConstantPositionMobilityModel> sta1Mobility = CreateObject<ConstantPositionMobilityModel> ();
  m_rxPhy->SetMobility (sta1Mobility);
  rxDev->SetPhy (m_rxPhy);
  rxNode->AggregateObject (sta1Mobility);
  rxNode->AddDevice (rxDev);
  m_rxPhy->TraceConnectWithoutContext ("PhyRxBegin", MakeCallback (&SpectrumWifiPhyFilterTest::RxCallback, this));
}

void
SpectrumWifiPhyFilterTest::RunOne (void)
{
  m_txPhy->SetChannelWidth (m_txChannelWidth);
  uint16_t txFrequency;
  switch (m_txChannelWidth)
    {
    case 20:
    default:
      txFrequency = 5180;
      break;
    case 40:
      txFrequency = 5190;
      break;
    case 80:
      txFrequency = 5210;
      break;
    case 160:
      txFrequency = 5250;
      break;
    }
  m_txPhy->SetFrequency (txFrequency);

  m_rxPhy->SetChannelWidth (m_rxChannelWidth);
  uint16_t rxFrequency;
  switch (m_rxChannelWidth)
    {
    case 20:
    default:
      rxFrequency = 5180;
      break;
    case 40:
      rxFrequency = 5190;
      break;
    case 80:
      rxFrequency = 5210;
      break;
    case 160:
      rxFrequency = 5250;
      break;
    }
  m_rxPhy->SetFrequency (rxFrequency);

  Simulator::Schedule (Seconds (1), &SpectrumWifiPhyFilterTest::SendPpdu, this);
  
  Simulator::Run ();
}

void
SpectrumWifiPhyFilterTest::DoRun (void)
{
  m_txChannelWidth = 20;
  m_rxChannelWidth = 20;
  RunOne ();

  m_txChannelWidth = 40;
  m_rxChannelWidth = 40;
  RunOne ();

  m_txChannelWidth = 80;
  m_rxChannelWidth = 80;
  RunOne ();

  m_txChannelWidth = 160;
  m_rxChannelWidth = 160;
  RunOne ();

  m_txChannelWidth = 20;
  m_rxChannelWidth = 40;
  RunOne ();

  m_txChannelWidth = 20;
  m_rxChannelWidth = 80;
  RunOne ();

  m_txChannelWidth = 40;
  m_rxChannelWidth = 80;
  RunOne ();

  m_txChannelWidth = 20;
  m_rxChannelWidth = 160;
  RunOne ();

  m_txChannelWidth = 40;
  m_rxChannelWidth = 160;
  RunOne ();

  m_txChannelWidth = 80;
  m_rxChannelWidth = 160;
  RunOne ();

  m_txChannelWidth = 40;
  m_rxChannelWidth = 20;
  RunOne ();

  m_txChannelWidth = 80;
  m_rxChannelWidth = 20;
  RunOne ();

  m_txChannelWidth = 80;
  m_rxChannelWidth = 40;
  RunOne ();

  m_txChannelWidth = 160;
  m_rxChannelWidth = 20;
  RunOne ();

  m_txChannelWidth = 160;
  m_rxChannelWidth = 40;
  RunOne ();

  m_txChannelWidth = 160;
  m_rxChannelWidth = 80;
  RunOne ();

  Simulator::Destroy ();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Spectrum Wifi Phy Test Suite
 */
class SpectrumWifiPhyTestSuite : public TestSuite
{
public:
  SpectrumWifiPhyTestSuite ();
};

SpectrumWifiPhyTestSuite::SpectrumWifiPhyTestSuite ()
  : TestSuite ("spectrum-wifi-phy", UNIT)
{
  AddTestCase (new SpectrumWifiPhyBasicTest, TestCase::QUICK);
  AddTestCase (new SpectrumWifiPhyListenerTest, TestCase::QUICK);
  AddTestCase (new SpectrumWifiPhyFilterTest, TestCase::QUICK);
}

static SpectrumWifiPhyTestSuite spectrumWifiPhyTestSuite; ///< the test suite
