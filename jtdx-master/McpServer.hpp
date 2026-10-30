#ifndef JTDX_FM_MCP_SERVER_HPP
#define JTDX_FM_MCP_SERVER_HPP
#include <QTcpServer>
#include <QJsonArray>
#include <QJsonObject>
#include <functional>

struct McpSettings {
  bool enabled = false;
  int port = 22380;
  QString token;
  bool allowControl = false;
  bool allowTransmit = false;
};

// All handlers run serially in the GUI thread, just like local button actions.
class McpServer : public QObject {
public:
  using Handler = std::function<QJsonObject(QString const&, QJsonObject const&)>;
  explicit McpServer(Handler handler, QObject *parent = nullptr);
  QString configure(McpSettings const& settings);
  bool listening() const { return server_.isListening(); }
  QJsonObject dispatch(QJsonObject const& request);
  static QJsonArray tools();
  static QJsonObject success(QJsonObject const& data);
  static QJsonObject failure(QString const& message);
private:
  void acceptConnections();
  QTcpServer server_;
  McpSettings settings_;
  Handler handler_;
};
#endif
