/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#define NOMINMAX

#include "server.h"

#include <QNetworkReply>
#include <QRandomGenerator>
#include <QSqlQuery>
#include <QUrlQuery>

#include "../Protocol/kp.h"
#include "kerrors.h"

QT_BEGIN_NAMESPACE

void Server::handleARDPurchaseAuth(const CSteamID &uid,
                                   QSslSocket *connection,
                                   const QJsonObject &djson) {
    quint64 orderId = djson["orderid"].toString().toULongLong();
    bool authorized = djson["authorized"].toBool();
    if(!authorized) {
        pendingARDOrders.remove(orderId);
        QByteArray msg = KP::serverARDPurchaseFailed(
            KP::PurchaseNotAuthorized);
        senderM.sendMessage(connection, msg);
        return;
    }
    if(!pendingARDOrders.contains(orderId)) {
        QByteArray msg = KP::serverARDPurchaseFailed(
            KP::PurchaseOrderNotFound);
        senderM.sendMessage(connection, msg);
        return;
    }
    auto [orderUid, units] = pendingARDOrders[orderId];
    if(orderUid != uid) {
        QByteArray msg = KP::serverARDPurchaseFailed(
            KP::PurchaseOrderMismatch);
        senderM.sendMessage(connection, msg);
        pendingARDOrders.remove(orderId);
        return;
    }
    int unitsToAdd = units;
    QNetworkRequest request(QUrl(QString(KP::microTxnBaseUrl)
                                 + QStringLiteral("FinalizeTxn/v2/")));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded");
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("key"),
                        settings->value("steam/webkey", "").toString());
    params.addQueryItem(QStringLiteral("orderid"), QString::number(orderId));
    params.addQueryItem(QStringLiteral("appid"),
                        QString::number(KP::steamAppId));
    QNetworkReply *reply = networkManager.post(
        request,
        params.toString(QUrl::FullyEncoded).toUtf8());
    pendingARDOrders.remove(orderId);
    connect(reply, &QNetworkReply::finished,
            this, [this, reply, uid, unitsToAdd, orderId]() {
                reply->deleteLater();
                try {
                QByteArray responseData = reply->readAll();
                QJsonDocument doc = QJsonDocument::fromJson(responseData);
                QJsonObject root = doc.object();
                QString result = root["response"]
                                     .toObject()["result"].toString();
                if(result == QStringLiteral("OK")) {
                    QSqlQuery query;
                    query.prepare(
                        "UPDATE UserAttr SET Intvalue = Intvalue + :units "
                        "WHERE Attribute = :attr AND UserID = :uid");
                    query.bindValue(":units", unitsToAdd);
                    query.bindValue(":attr", KP::attrARDCoupon);
                    query.bindValue(":uid", uid.ConvertToUint64());
                    if(Q_UNLIKELY(!query.exec())) {
                        qCritical() << query.lastError();
                        if(connectedPeers.contains(uid)) {
                            QByteArray msg = KP::serverARDPurchaseFailed(
                                KP::PurchaseDatabaseError);
                            senderM.sendMessage(connectedPeers[uid], msg);
                        }
                        return;
                    }
                    QSqlQuery orderRecord;
                    orderRecord.prepare(
                        "INSERT OR IGNORE INTO ARDOrders "
                        "(OrderID, UserID, Units, Status) "
                        "VALUES (:oid, :uid, :units, 'active')");
                    orderRecord.bindValue(":oid", orderId);
                    orderRecord.bindValue(":uid",
                                          QString::number(uid.ConvertToUint64()));
                    orderRecord.bindValue(":units", unitsToAdd);
                    if(Q_UNLIKELY(!orderRecord.exec())) {
                        //% "Store purchase info failed! User %1, orderid %2"
                        throw DBError(qtTrId("store-purchase-info-failed")
                                          .arg(uid.ConvertToUint64())
                                          .arg(orderId),
                                      query.lastError(), query.lastQuery());
                    }
                    if(connectedPeers.contains(uid)) {
                        QByteArray msg = KP::serverARDPurchaseSuccess(unitsToAdd);
                        senderM.sendMessage(connectedPeers[uid], msg);
                        offerResourceInfo(connectedPeers[uid], uid);
                    }
                }
                else {
                    QJsonObject root2 = doc.object();
                    QString errDesc = root2["response"]
                                          .toObject()["error"]
                                          .toObject()["errordesc"].toString();
                    qWarning() << "FinalizeTxn Steam error:" << errDesc;
                    if(connectedPeers.contains(uid)) {
                        QByteArray msg = KP::serverARDPurchaseFailed(
                            KP::PurchaseSteamError);
                        senderM.sendMessage(connectedPeers[uid], msg);
                    }
                }
                } catch (DBError &e) {
                    for(QString &i : e.whats()) {
                        qCritical() << i;
                    }
                }
            });
}

void Server::handleInitARDPurchase(const CSteamID &uid,
                                   QSslSocket *connection,
                                   int units) {
    if(units < 1 || units >= KP::ardCouponMaxUnits) {
        QByteArray msg = KP::serverARDPurchaseFailed(
            KP::PurchaseInvalidAmount);
        senderM.sendMessage(connection, msg);
        return;
    }
    int priceHKDCents = KP::ardRealPriceHKDCents(units);
    quint64 orderId;
    do {
        orderId = QRandomGenerator::global()->generate64();
    } while(orderId == 0 || pendingARDOrders.contains(orderId));
    QNetworkRequest request(QUrl(QString(KP::microTxnBaseUrl)
                                 + QStringLiteral("InitTxn/v3/")));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded");
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("key"),
                        settings->value("steam/webkey", "").toString());
    params.addQueryItem(QStringLiteral("orderid"), QString::number(orderId));
    params.addQueryItem(QStringLiteral("steamid"),
                        QString::number(uid.ConvertToUint64()));
    params.addQueryItem(QStringLiteral("appid"),
                        QString::number(KP::steamAppId));
    params.addQueryItem(QStringLiteral("itemcount"), QStringLiteral("1"));
    params.addQueryItem(QStringLiteral("language"), QStringLiteral("en"));
    params.addQueryItem(QStringLiteral("currency"), QStringLiteral("HKD"));
    params.addQueryItem(QStringLiteral("usersession"),
                        QStringLiteral("client"));
    params.addQueryItem(QStringLiteral("ipaddress"),
                        connection->peerAddress().toString());
    params.addQueryItem(QStringLiteral("itemid[0]"),
                        QString::number(KP::ardCouponItemId));
    params.addQueryItem(QStringLiteral("qty[0]"), QStringLiteral("1"));
    params.addQueryItem(QStringLiteral("amount[0]"),
                        QString::number(priceHKDCents));
    params.addQueryItem(QStringLiteral("description[0]"),
                        //% "%1 ARD Coupons"
                        qtTrId("ard-coupon-description").arg(units));
    QNetworkReply *reply = networkManager.post(
        request,
        params.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished,
            this, [this, reply, uid, orderId, units]() {
                reply->deleteLater();
                QByteArray responseData = reply->readAll();
                QJsonDocument doc = QJsonDocument::fromJson(responseData);
                QJsonObject root3 = doc.object();
                QString result = root3["response"]
                                     .toObject()["result"].toString();
                if(result == QStringLiteral("OK")) {
                    pendingARDOrders[orderId] = {uid, units};
                    if(connectedPeers.contains(uid)) {
                        QByteArray msg = KP::serverARDPurchasePending(orderId);
                        senderM.sendMessage(connectedPeers[uid], msg);
                    }
                }
                else {
                    QJsonObject root4 = doc.object();
                    QString errDesc = root4["response"]
                                          .toObject()["error"]
                                          .toObject()["errordesc"].toString();
                    qWarning() << "InitTxn Steam error:" << errDesc;
                    if(connectedPeers.contains(uid)) {
                        QByteArray msg = KP::serverARDPurchaseFailed(
                            KP::PurchaseSteamError);
                        senderM.sendMessage(connectedPeers[uid], msg);
                    }
                }
            });
}

void Server::pollARDRefunds() {
    QDateTime lastPollTime
        = settings->value("steam/lastrefundpolltime",
                          QDateTime::currentDateTimeUtc()).toDateTime();
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("key"),
                        settings->value("steam/webkey", "").toString());
    params.addQueryItem(QStringLiteral("appid"),
                        QString::number(KP::steamAppId));
    params.addQueryItem(QStringLiteral("type"),
                        QStringLiteral("SETTLEMENT"));
    params.addQueryItem(QStringLiteral("time"),
                        lastPollTime.toString(Qt::ISODate));
    params.addQueryItem(QStringLiteral("maxresults"),
                        QStringLiteral("100"));
    QUrl url(QString(KP::microTxnBaseUrl)
             + QStringLiteral("GetReport/v5/"));
    url.setQuery(params);
    QNetworkReply *reply = networkManager.get(QNetworkRequest(url));
    settings->setValue("steam/lastrefundpolltime",
                       QDateTime::currentDateTimeUtc());
    connect(reply, &QNetworkReply::finished,
            this, [this, reply]() {
                reply->deleteLater();
                try {
                check_response:
                    QByteArray responseData = reply->readAll();
                    QJsonDocument doc = QJsonDocument::fromJson(responseData);
                    QJsonObject root5 = doc.object();
                    QJsonObject resp = root5["response"].toObject();
                    if(resp["result"].toString() != QStringLiteral("OK")) {
                        //% "ARD refund poll failed: %1"
                        qWarning() << qtTrId("ard-refund-poll-failed")
                                          .arg(resp["error"]
                                                   .toObject()["errordesc"].toString());
                        return;
                    }
                process_transactions:
                    static const QSet<QString> reversedStatuses = {
                        QStringLiteral("Refunded"),
                        QStringLiteral("PartialRefund"),
                        QStringLiteral("Chargedback"),
                        QStringLiteral("RefundedSuspectedFraud"),
                        QStringLiteral("RefundedFriendlyFraud"),
                    };
                    QJsonArray transactions = resp["transactions"].toArray();
                    for(const QJsonValue &txVal : std::as_const(transactions)) {
                        QJsonObject tx = txVal.toObject();
                        if(!reversedStatuses.contains(tx["status"].toString())) {
                            continue;
                        }
                        quint64 orderId = tx["orderid"].toString().toULongLong();
                        QSqlQuery lookup;
                        lookup.prepare("SELECT UserID, Units FROM ARDOrders "
                                       "WHERE OrderID = :oid AND Status = 'active'");
                        lookup.bindValue(":oid", orderId);
                        if(Q_UNLIKELY(!lookup.exec())) {
                            //% "ARD clawback: order lookup failed"
                            throw DBError(qtTrId("ard-clawback-lookup-failed"),
                                          lookup.lastError(), lookup.lastQuery());
                        }
                        if(!lookup.next()) {
                            continue; // not our order or already processed
                        }
                        quint64 rawUid = lookup.value(0).toULongLong();
                        int units = lookup.value(1).toInt();
                        CSteamID uid(rawUid);
                        QSqlQuery deduct;
                        deduct.prepare("UPDATE UserAttr "
                                       // can go below 0
                                       "SET Intvalue = Intvalue - :units "
                                       "WHERE UserID = :uid AND Attribute = :attr");
                        deduct.bindValue(":units", units);
                        deduct.bindValue(":uid", rawUid);
                        deduct.bindValue(":attr", KP::attrARDCoupon);
                        if(Q_UNLIKELY(!deduct.exec())) {
                            //% "ARD clawback: deduct failed for order %1"
                            throw DBError(qtTrId("ard-clawback-deduct-failed")
                                              .arg(orderId), deduct.lastError());
                        }
                        QSqlQuery mark;
                        mark.prepare("UPDATE ARDOrders SET Status = 'clawedback' "
                                     "WHERE OrderID = :oid");
                        mark.bindValue(":oid", orderId);
                        if(Q_UNLIKELY(!mark.exec())) {
                            //% "ARD clawback: mark failed for order %1"
                            throw DBError(qtTrId("ard-clawback-mark-failed")
                                              .arg(orderId), mark.lastError());
                        }
                        qWarning() << "ARD clawback: order" << orderId
                                   << "user" << rawUid
                                   << "units" << units
                                   << "status" << tx["status"].toString();
                        if(connectedPeers.contains(uid)) {
                            QByteArray msg = KP::serverARDPurchaseClawback(units);
                            senderM.sendMessage(connectedPeers[uid], msg);
                            offerResourceInfo(connectedPeers[uid], uid);
                        }
                    }
                } catch (DBError &e) {
                    for(QString &i : e.whats()) {
                        qCritical() << i;
                    }
                } catch (std::exception &e) {
                    qCritical() << e.what();
                }
            });
}

QT_END_NAMESPACE
