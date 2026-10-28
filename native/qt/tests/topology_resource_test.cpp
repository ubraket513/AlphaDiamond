#include "native_controller.hpp"
#include "soo/board.hpp"

#include <QAbstractItemModel>
#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>

int main(int argc, char** argv) {
    QTemporaryDir isolated;
    if (!isolated.isValid() || !QDir::setCurrent(isolated.path())) return 1;
    qputenv("XDG_DATA_HOME", isolated.path().toUtf8());
    qputenv("XDG_CONFIG_HOME", isolated.path().toUtf8());
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("AlphaDiamondTest"));
    QCoreApplication::setApplicationName(QStringLiteral("TopologyResourceTest"));
    NativeController controller;
    controller.startMatch(QVariantList{1, 2}, QVariantList{});
    auto* board = qobject_cast<QAbstractItemModel*>(controller.boardModel());
    auto* pieces = qobject_cast<QAbstractItemModel*>(controller.pieceModel());
    if (!soo::topology().configured || !board || board->rowCount() != soo::kBoardSize ||
        !pieces || pieces->rowCount() != 2 * soo::kCampSize) {
        qCritical("No-model startup did not produce a configured board and pieces");
        return 1;
    }
    return 0;
}
