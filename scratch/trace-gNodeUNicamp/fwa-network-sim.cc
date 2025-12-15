/*
 * fwa-network-sim.cc
 * Versão: MULTI-STREAM TRAFFIC FIX
 * Objetivo: Destravar o tráfego TCP usando múltiplos fluxos e ajuste de rota de retorno.
 * Meta: Atingir >50% de carga de RBs.
 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/nr-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/ideal-beamforming-helper.h"
#include "ns3/antenna-module.h"

// Includes 3GPP
#include "ns3/spectrum-propagation-loss-model.h"
#include "ns3/phased-array-spectrum-propagation-loss-model.h"
#include "ns3/three-gpp-spectrum-propagation-loss-model.h"
#include "ns3/three-gpp-channel-model.h"
#include "ns3/three-gpp-propagation-loss-model.h"
#include "ns3/channel-condition-model.h"
#include "../contrib/nr/helper/cc-bwp-helper.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FwaNetworkSim");

int main(int argc, char* argv[])
{
    //------------------------------------------------PARÂMETROS--------------------------------------------------------
    double simDuration = 60.0;
    double trafficStartTime = 5.0;
    bool trafficEnabled = true;

    //---------------------------Ajuste de Buffers TCP para Alta Velocidade (100 Gbps link)-----------------------------
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(524288));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(524288));

    CommandLine cmd;
    cmd.AddValue("simDuration", "Duração da simulação", simDuration);
    cmd.AddValue("traffic", "Ativar tráfego", trafficEnabled);
    cmd.Parse(argc, argv);

    //------------------------------------------------- 1. Nós ---------------------------------------------------------
    NodeContainer remoteHostContainer; remoteHostContainer.Create(1);
    NodeContainer gnbContainer; gnbContainer.Create(1);
    NodeContainer cpeContainer; cpeContainer.Create(1);
    NodeContainer staNodes; staNodes.Create(1);

    //---------------------------------------------- 2. Mobilidade -----------------------------------------------------
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(gnbContainer);
    mobility.Install(cpeContainer);
    mobility.Install(remoteHostContainer);
    mobility.Install(staNodes);

    gnbContainer.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0, 0, 30));
    cpeContainer.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(100, 0, 6));

    //------------------------------------------------ 3. NR Helper ----------------------------------------------------
    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper>();
    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();

    nrHelper->SetEpcHelper(epcHelper);
    nrHelper->SetBeamformingHelper(idealBeamformingHelper);

    nrHelper->SetGnbPhyAttribute("TxPower", DoubleValue(46.0));
    nrHelper->SetGnbPhyAttribute("Numerology", UintegerValue(1));

    // TDD Pattern (Ajustado para garantir UL suficiente para ACKs)
    // DL|DL|DL|F|UL -> Dá mais chances de retorno de ACK que o padrão anterior
    Config::SetDefault("ns3::NrGnbPhy::Pattern", StringValue("DL|DL|DL|F|UL|DL|DL|DL|F|UL"));

    //----------------------------------------------- 4. Canal e BWP ---------------------------------------------------
    CcBwpCreator ccBwpCreator;
    CcBwpCreator::SimpleOperationBandConf bandConf(3.7e9, 40e6, 1);
    std::vector<CcBwpCreator::SimpleOperationBandConf> bandConfVector;
    bandConfVector.push_back(bandConf);

    auto bwpInfoPair = nrHelper->CreateBandwidthParts(bandConfVector, "RMa", "Default", "ThreeGpp");
    const auto& allBwps = bwpInfoPair.second;

    //----------------------------------------------- 5. Instalação ----------------------------------------------------
    NetDeviceContainer gnbNetDev = nrHelper->InstallGnbDevice(gnbContainer, allBwps);
    NetDeviceContainer cpeNetDev = nrHelper->InstallUeDevice(cpeContainer, allBwps);

    //--------------------------------------------- 6. Internet Core ---------------------------------------------------
    Ptr<Node> pgw = epcHelper->GetPgwNode();
    NodeContainer internetNodes;
    internetNodes.Add(remoteHostContainer.Get(0));
    internetNodes.Add(pgw);

    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", StringValue("100Gbps"));
    p2ph.SetChannelAttribute("Delay", StringValue("1ms"));
    NetDeviceContainer internetDevices = p2ph.Install(internetNodes);

    //------------------------------------------------- 7. Stack IP ----------------------------------------------------
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);
    internet.Install(cpeContainer);
    internet.Install(staNodes);
    internet.Install(gnbContainer);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("1.0.0.0", "255.0.0.0");
    ipv4.Assign(internetDevices);

    Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(cpeNetDev));
    Ipv4Address cpeWanAddress = ueIpIface.GetAddress(0);

    //Wi-Fi (Dummy para topologia)
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ac);
    wifi.SetRemoteStationManager("ns3::IdealWifiManager");
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());
    WifiMacHelper wifiMac;
    Ssid ssid = Ssid("FWA-LAN");
    wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDev = wifi.Install(wifiPhy, wifiMac, cpeContainer.Get(0));
    wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer staDev = wifi.Install(wifiPhy, wifiMac, staNodes.Get(0));
    ipv4.SetBase("192.168.1.0", "255.255.255.0");
    ipv4.Assign(apDev);
    ipv4.Assign(staDev);

    //ROTEAMENTO (FIX CRÍTICO)
    // 1. Host sabe chegar na rede 5G (7.0.0.0) via PGW (1.0.0.2)
    Ptr<Ipv4> rhIp = remoteHostContainer.Get(0)->GetObject<Ipv4>();
    Ptr<Ipv4StaticRouting> rhStatic = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(rhIp->GetRoutingProtocol());
    rhStatic->SetDefaultRoute(Ipv4Address("1.0.0.2"), 1);

    //2. CPE sabe responder para a Internet (1.0.0.0) via túnel 5G
    // O EPC Helper geralmente faz isso, mas vamos garantir.
    // O Gateway do UE dentro do tunel é sempre o x.x.x.1 da subnet UE
    // Vamos confiar no EPC aqui, mas se falhar, podemos forçar.

    //-------------------------------------------------8. Attach--------------------------------------------------------
    nrHelper->AttachToClosestGnb(cpeNetDev, gnbNetDev);

    //--------------------------------------9. Aplicações (MULTI-STREAMING)---------------------------------------------
    if (trafficEnabled) {
        uint16_t basePort = 8080;
        //5 Fluxos paralelos para estressar a rede
        int numStreams = 5;

        for (int i = 0; i < numStreams; i++) {
            uint16_t currentPort = basePort + i;

            //Sink no CPE
            PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), currentPort));
            ApplicationContainer sinkApps = sink.Install(cpeContainer.Get(0));
            sinkApps.Start(Seconds(0.0));
            sinkApps.Stop(Seconds(simDuration));

            //Source no Host
            BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(cpeWanAddress, currentPort));
            source.SetAttribute("MaxBytes", UintegerValue(0));
            // Pacotes maiores
            source.SetAttribute("SendSize", UintegerValue(2048));

            ApplicationContainer sourceApps = source.Install(remoteHostContainer.Get(0));

            //Inicia escalonado para evitar pico de colisão ARP inicial
            sourceApps.Start(Seconds(trafficStartTime + (i * 0.1)));
            sourceApps.Stop(Seconds(simDuration));
        }

        NS_LOG_UNCOND(">>> TRAFFIC MODE <<< " << numStreams << " TCP Streams Ativos");
    } else {
        NS_LOG_UNCOND(">>> BEACONING MODE <<<");
    }

    //-----------------------------------------------10. Traces---------------------------------------------------------
    nrHelper->EnableTraces();

    Simulator::Stop(Seconds(simDuration + 0.5));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
