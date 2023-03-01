/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "my-tcp-echo-helper.h"
#include "ns3/my-tcp-echo-server.h"
#include "ns3/my-tcp-echo-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

    MyTcpEchoServerHelper::MyTcpEchoServerHelper (Address address, uint16_t port)
    {
        m_factory.SetTypeId (MyTcpEchoServer::GetTypeId ());
        SetAttribute ("Local", AddressValue (address));
        SetAttribute ("Port", UintegerValue (port));
    }

    MyTcpEchoServerHelper::MyTcpEchoServerHelper (Address address, uint16_t port,u_int32_t mss)
    {
        m_factory.SetTypeId (MyTcpEchoServer::GetTypeId ());
        SetAttribute ("Local", AddressValue (address));
        SetAttribute ("Port", UintegerValue (port));
        SetAttribute ("MSS", UintegerValue (mss));
    }

    MyTcpEchoServerHelper::MyTcpEchoServerHelper ( uint16_t port)
    {
        m_factory.SetTypeId (MyTcpEchoServer::GetTypeId ());
        SetAttribute ("Port", UintegerValue (port));
    }

    void
    MyTcpEchoServerHelper::SetAttribute (
            std::string name,
            const AttributeValue &value)
    {
        m_factory.Set (name, value);
    }

    ApplicationContainer
    MyTcpEchoServerHelper::Install (Ptr<Node> node) const
    {
        return ApplicationContainer (InstallPriv (node));
    }

    ApplicationContainer
    MyTcpEchoServerHelper::Install (std::string nodeName) const
    {
        Ptr<Node> node = Names::Find<Node> (nodeName);
        return ApplicationContainer (InstallPriv (node));
    }

    ApplicationContainer
    MyTcpEchoServerHelper::Install (NodeContainer c) const
    {
        ApplicationContainer apps;
        for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
        {
            apps.Add (InstallPriv (*i));
        }

        return apps;
    }

    Ptr<Application>
    MyTcpEchoServerHelper::InstallPriv (Ptr<Node> node) const
    {
        Ptr<Application> app = m_factory.Create<MyTcpEchoServer> ();
        node->AddApplication (app);

        return app;
    }

    void MyTcpEchoServerHelper::SetPacketSizes (Ptr<Application> app, uint32_t *fill)
    {
        app->GetObject<MyTcpEchoServer>()->SetPacketSizes (fill);
    }


    MyTcpEchoClientHelper::MyTcpEchoClientHelper (Address address, uint16_t port)
    {
        m_factory.SetTypeId (MyTcpEchoClient::GetTypeId ());
        SetAttribute ("RemoteAddress", AddressValue (address));
        SetAttribute ("RemotePort", UintegerValue (port));
    }

    void
    MyTcpEchoClientHelper::SetAttribute (
            std::string name,
            const AttributeValue &value)
    {
        m_factory.Set (name, value);
    }


    void MyTcpEchoClientHelper::SetPacketSizes (Ptr<Application> app, uint32_t *fill)
    {
        app->GetObject<MyTcpEchoClient>()->SetPacketSizes (fill);
    }

    void
    MyTcpEchoClientHelper::SetFill (Ptr<Application> app, std::string fill)
    {
        app->GetObject<MyTcpEchoClient>()->SetFill (fill);
    }

    void
    MyTcpEchoClientHelper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
    {
        app->GetObject<MyTcpEchoClient>()->SetFill (fill, dataLength);
    }

    void
    MyTcpEchoClientHelper::SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength)
    {
        app->GetObject<MyTcpEchoClient>()->SetFill (fill, fillLength, dataLength);
    }

    ApplicationContainer
    MyTcpEchoClientHelper::Install (Ptr<Node> node) const
    {
        return ApplicationContainer (InstallPriv (node));
    }

    ApplicationContainer
    MyTcpEchoClientHelper::Install (std::string nodeName) const
    {
        Ptr<Node> node = Names::Find<Node> (nodeName);
        return ApplicationContainer (InstallPriv (node));
    }

    ApplicationContainer
    MyTcpEchoClientHelper::Install (NodeContainer c) const
    {
        ApplicationContainer apps;
        for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
        {
            apps.Add (InstallPriv (*i));
        }

        return apps;
    }

    Ptr<Application>
    MyTcpEchoClientHelper::InstallPriv (Ptr<Node> node) const
    {
        Ptr<Application> app = m_factory.Create<MyTcpEchoClient> ();
        node->AddApplication (app);

        return app;
    }

} // namespace ns3
