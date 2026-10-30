#include "McpServer.hpp"
#include <QCoreApplication>
#include <QTimer>
#include <cstdio>
int main(int argc, char** argv) {
  QCoreApplication app(argc, argv);
  McpServer server([](QString const& name, QJsonObject const&) {
    return McpServer::success({{"application", "JTDX-F-fixture"}, {"tool", name}, {"tx_enabled", false}});
  });
  McpSettings settings;
  settings.enabled=true; settings.allowControl=true;
  settings.port=app.arguments().value(1).toInt();
  settings.token=qEnvironmentVariable("JTDX_MCP_TOKEN");
  if (!server.configure(settings).isEmpty()) return 2;
  puts("ready"); fflush(stdout);
  QTimer::singleShot(60000,&app,&QCoreApplication::quit);
  return app.exec();
}
