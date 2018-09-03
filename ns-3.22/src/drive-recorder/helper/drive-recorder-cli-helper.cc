#include "drive-recorder-cli-helper.h"

namespace ns3 {

ClientApplicationHelper::ClientApplicationHelper () {
  m_factory.SetTypeId ("ns3::ClientApplication");
}

void ClientApplicationHelper::SetAttribute (std::string name, const AttributeValue &value) {
  m_factory.Set (name, value);
}

ApplicationContainer ClientApplicationHelper::Install (Ptr<Node> node) const {
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer ClientApplicationHelper::Install (NodeContainer c) const {
  ApplicationContainer apps;
  for (NodeContainer::Iterator idx = c.Begin (); idx != c.End (); ++idx) {
    apps.Add (InstallPriv (*idx));
  }
  return apps;
}

Ptr<Application> ClientApplicationHelper::InstallPriv (Ptr<Node> node) const {
  Ptr<Application> app = m_factory.Create<Application> ();
  node->AddApplication (app);

  return app;
}

}

