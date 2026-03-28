/**
 * @file steam_microtxn.cpp
 * @brief Steam ISteamMicroTxn API implementation for Qt/C++
 *
 * Implements all endpoints: InitTxn, GetUserInfo, FinalizeTxn, QueryTxn,
 * RefundTxn, GetReport, CancelAgreement, AdjustAgreement, ProcessAgreement,
 * GetUserAgreementInfo.
 *
 * @author MiniMax Agent
 * @date 2026-03-28
 */

#include "steam_microtxn.h"

// ============================================================================
// Constants
// ============================================================================

const QString STEAM_API_BASE_URL = QStringLiteral("https://partner.steam-api.com/ISteamMicroTxn");

// ============================================================================
// SteamMicroTxn Implementation
// ============================================================================

SteamMicroTxn::SteamMicroTxn(const QString &apiKey, QObject *parent)
    : QObject(parent)
    , m_apiKey(apiKey)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &SteamMicroTxn::onReplyFinished);
}

SteamMicroTxn::~SteamMicroTxn()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }
}

// ============================================================================
// Configuration
// ============================================================================

void SteamMicroTxn::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

QString SteamMicroTxn::apiKey() const
{
    return m_apiKey;
}

bool SteamMicroTxn::isValid() const
{
    return !m_apiKey.isEmpty();
}

QString SteamMicroTxn::baseUrl()
{
    return STEAM_API_BASE_URL;
}

// ============================================================================
// GetUserInfo (GET v2)
// ============================================================================

void SteamMicroTxn::getUserInfo(quint64 appId, quint64 steamId, const QString &ipAddress)
{
    if (!isValid()) {
        SteamUserInfo errorInfo;
        errorInfo.success = false;
        errorInfo.errorCode = QStringLiteral("INVALID_API_KEY");
        errorInfo.errorDesc = QStringLiteral("API key is not set or empty");
        emit getUserInfoFinished(errorInfo);
        return;
    }

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("key"), m_apiKey);
    query.addQueryItem(QStringLiteral("appid"), QString::number(appId));

    if (steamId > 0) {
        query.addQueryItem(QStringLiteral("steamid"), QString::number(steamId));
    }

    if (!ipAddress.isEmpty()) {
        query.addQueryItem(QStringLiteral("ipaddress"), ipAddress);
    }

    sendGetRequest(QStringLiteral("GetUserInfo/v2/"), query);
    m_currentEndpoint = QStringLiteral("GetUserInfo");
}

// ============================================================================
// InitTxn (POST v3)
// ============================================================================

void SteamMicroTxn::initTxn(quint64 orderId, quint64 steamId, quint32 appId,
                            const QVector<SteamItemParams> &items,
                            const QString &language, const QString &currency,
                            const QString &userSession,
                            const QString &ipAddress,
                            const QVector<SteamBundleParams> &bundles)
{
    if (!isValid()) {
        SteamInitTxnResponse errorResp;
        errorResp.success = false;
        errorResp.errorCode = QStringLiteral("INVALID_API_KEY");
        errorResp.errorDesc = QStringLiteral("API key is not set or empty");
        emit initTxnFinished(errorResp);
        return;
    }

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("key"), m_apiKey);
    params.addQueryItem(QStringLiteral("orderid"), QString::number(orderId));
    params.addQueryItem(QStringLiteral("steamid"), QString::number(steamId));
    params.addQueryItem(QStringLiteral("appid"), QString::number(appId));
    params.addQueryItem(QStringLiteral("itemcount"), QString::number(items.size()));
    params.addQueryItem(QStringLiteral("language"), language);
    params.addQueryItem(QStringLiteral("currency"), currency);

    if (!userSession.isEmpty()) {
        params.addQueryItem(QStringLiteral("usersession"), userSession);
    }

    if (!ipAddress.isEmpty()) {
        params.addQueryItem(QStringLiteral("ipaddress"), ipAddress);
    }

    // Add item parameters
    for (int i = 0; i < items.size(); ++i) {
        const SteamItemParams &item = items.at(i);
        QString prefix = QStringLiteral("itemid[%1]").arg(i);
        params.addQueryItem(prefix, QString::number(item.itemId));
        params.addQueryItem(QStringLiteral("qty[%1]").arg(i), QString::number(item.qty));
        params.addQueryItem(QStringLiteral("amount[%1]").arg(i), QString::number(item.amount));
        params.addQueryItem(QStringLiteral("description[%1]").arg(i), item.description);

        if (!item.category.isEmpty()) {
            params.addQueryItem(QStringLiteral("category[%1]").arg(i), item.category);
        }
        if (!item.billingType.isEmpty()) {
            params.addQueryItem(QStringLiteral("billingtype[%1]").arg(i), item.billingType);
        }
        if (!item.period.isEmpty()) {
            params.addQueryItem(QStringLiteral("period[%1]").arg(i), item.period);
        }
        if (item.frequency > 0) {
            params.addQueryItem(QStringLiteral("frequency[%1]").arg(i), QString::number(item.frequency));
        }
        if (item.recurringAmt > 0) {
            params.addQueryItem(QStringLiteral("recurringamt[%1]").arg(i), QString::number(item.recurringAmt));
        }
        if (item.associatedBundle > 0) {
            params.addQueryItem(QStringLiteral("associated_bundle[%1]").arg(i), QString::number(item.associatedBundle));
        }
    }

    // Add bundle parameters
    if (!bundles.isEmpty()) {
        params.addQueryItem(QStringLiteral("bundlecount"), QString::number(bundles.size()));
        for (int i = 0; i < bundles.size(); ++i) {
            const SteamBundleParams &bundle = bundles.at(i);
            params.addQueryItem(QStringLiteral("bundleid[%1]").arg(i), QString::number(bundle.bundleId));
            params.addQueryItem(QStringLiteral("bundle_qty[%1]").arg(i), QString::number(bundle.qty));
            params.addQueryItem(QStringLiteral("bundle_desc[%1]").arg(i), bundle.description);
            if (!bundle.category.isEmpty()) {
                params.addQueryItem(QStringLiteral("bundle_category[%1]").arg(i), bundle.category);
            }
        }
    }

    sendPostRequest(QStringLiteral("InitTxn/v3/"), params);
    m_currentEndpoint = QStringLiteral("InitTxn");
}

// ============================================================================
// FinalizeTxn (POST v2)
// ============================================================================

void SteamMicroTxn::finalizeTxn(quint64 orderId, quint32 appId)
{
    if (!isValid()) {
        SteamFinalizeTxnResponse errorResp;
        errorResp.success = false;
        errorResp.errorCode = QStringLiteral("INVALID_API_KEY");
        errorResp.errorDesc = QStringLiteral("API key is not set or empty");
        emit finalizeTxnFinished(errorResp);
        return;
    }

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("key"), m_apiKey);
    params.addQueryItem(QStringLiteral("orderid"), QString::number(orderId));
    params.addQueryItem(QStringLiteral("appid"), QString::number(appId));

    sendPostRequest(QStringLiteral("FinalizeTxn/v2/"), params);
    m_currentEndpoint = QStringLiteral("FinalizeTxn");
}

// ============================================================================
// QueryTxn (GET v3)
// ============================================================================

void SteamMicroTxn::queryTxn(quint32 appId, quint64 orderId, quint64 transId)
{
    if (!isValid()) {
        SteamQueryTxnResponse errorResp;
        errorResp.success = false;
        errorResp.errorCode = QStringLiteral("INVALID_API_KEY");
        errorResp.errorDesc = QStringLiteral("API key is not set or empty");
        emit queryTxnFinished(errorResp);
        return;
    }

    if (orderId == 0 && transId == 0) {
        SteamQueryTxnResponse errorResp;
        errorResp.success = false;
        errorResp.errorCode = QStringLiteral("INVALID_PARAMS");
        errorResp.errorDesc = QStringLiteral("Either orderid or transid must be provided");
        emit queryTxnFinished(errorResp);
        return;
    }

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("key"), m_apiKey);
    params.addQueryItem(QStringLiteral("appid"), QString::number(appId));

    if (orderId > 0) {
        params.addQueryItem(QStringLiteral("orderid"), QString::number(orderId));
    }
    if (transId > 0) {
        params.addQueryItem(QStringLiteral("transid"), QString::number(transId));
    }

    sendGetRequest(QStringLiteral("QueryTxn/v3/"), params);
    m_currentEndpoint = QStringLiteral("QueryTxn");
}

// ============================================================================
// RefundTxn (POST v2)
// ============================================================================

void SteamMicroTxn::refundTxn(quint64 orderId, quint32 appId)
{
    if (!isValid()) {
        SteamRefundTxnResponse errorResp;
        errorResp.success = false;
        errorResp.errorCode = QStringLiteral("INVALID_API_KEY");
        errorResp.errorDesc = QStringLiteral("API key is not set or empty");
        emit refundTxnFinished(errorResp);
        return;
    }

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("key"), m_apiKey);
    params.addQueryItem(QStringLiteral("orderid"), QString::number(orderId));
    params.addQueryItem(QStringLiteral("appid"), QString::number(appId));

    sendPostRequest(QStringLiteral("RefundTxn/v2/"), params);
    m_currentEndpoint = QStringLiteral("RefundTxn");
}

// ============================================================================
// GetReport (GET v5)
// ============================================================================

void SteamMicroTxn::getReport(quint32 appId, const QString &startTime,
                               SteamReportType type, quint32 maxResults)
{
    if (!isValid()) {
        SteamGetReportResponse errorResp;
        errorResp.success = false;
        errorResp.errorCode = QStringLiteral("INVALID_API_KEY");
        errorResp.errorDesc = QStringLiteral("API key is not set or empty");
        emit getReportFinished(errorResp);
        return;
    }

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("key"), m_apiKey);
    params.addQueryItem(QStringLiteral("appid"), QString::number(appId));
    params.addQueryItem(QStringLiteral("time"), startTime);
    params.addQueryItem(QStringLiteral("type"), reportTypeToString(type));
    params.addQueryItem(QStringLiteral("maxresults"), QString::number(maxResults));

    sendGetRequest(QStringLiteral("GetReport/v5/"), params);
    m_currentEndpoint = QStringLiteral("GetReport");
}

// ============================================================================
// GetUserAgreementInfo (GET v2)
// ============================================================================

void SteamMicroTxn::getUserAgreementInfo(quint64 steamId, quint32 appId)
{
    if (!isValid()) {
        SteamGetUserAgreementInfoResponse errorResp;
        errorResp.success = false;
        errorResp.errorCode = QStringLiteral("INVALID_API_KEY");
        errorResp.errorDesc = QStringLiteral("API key is not set or empty");
        emit getUserAgreementInfoFinished(errorResp);
        return;
    }

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("key"), m_apiKey);
    params.addQueryItem(QStringLiteral("steamid"), QString::number(steamId));
    params.addQueryItem(QStringLiteral("appid"), QString::number(appId));

    sendGetRequest(QStringLiteral("GetUserAgreementInfo/v2/"), params);
    m_currentEndpoint = QStringLiteral("GetUserAgreementInfo");
}

// ============================================================================
// CancelAgreement (POST v1)
// ============================================================================

void SteamMicroTxn::cancelAgreement(quint64 steamId, quint64 agreementId, quint32 appId)
{
    if (!isValid()) {
        SteamCancelAgreementResponse errorResp;
        errorResp.success = false;
        errorResp.errorCode = QStringLiteral("INVALID_API_KEY");
        errorResp.errorDesc = QStringLiteral("API key is not set or empty");
        emit cancelAgreementFinished(errorResp);
        return;
    }

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("key"), m_apiKey);
    params.addQueryItem(QStringLiteral("steamid"), QString::number(steamId));
    params.addQueryItem(QStringLiteral("agreementid"), QString::number(agreementId));
    params.addQueryItem(QStringLiteral("appid"), QString::number(appId));

    sendPostRequest(QStringLiteral("CancelAgreement/v1/"), params);
    m_currentEndpoint = QStringLiteral("CancelAgreement");
}

// ============================================================================
// AdjustAgreement (POST v1)
// ============================================================================

void SteamMicroTxn::adjustAgreement(quint64 steamId, quint64 agreementId, quint32 appId,
                                     const QString &nextProcessDate,
                                     const QString &oldNextProcessDate)
{
    if (!isValid()) {
        SteamAdjustAgreementResponse errorResp;
        errorResp.success = false;
        errorResp.errorCode = QStringLiteral("INVALID_API_KEY");
        errorResp.errorDesc = QStringLiteral("API key is not set or empty");
        emit adjustAgreementFinished(errorResp);
        return;
    }

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("key"), m_apiKey);
    params.addQueryItem(QStringLiteral("steamid"), QString::number(steamId));
    params.addQueryItem(QStringLiteral("agreementid"), QString::number(agreementId));
    params.addQueryItem(QStringLiteral("appid"), QString::number(appId));
    params.addQueryItem(QStringLiteral("nextprocessdate"), nextProcessDate);

    if (!oldNextProcessDate.isEmpty()) {
        params.addQueryItem(QStringLiteral("oldnextprocessdate"), oldNextProcessDate);
    }

    sendPostRequest(QStringLiteral("AdjustAgreement/v1/"), params);
    m_currentEndpoint = QStringLiteral("AdjustAgreement");
}

// ============================================================================
// ProcessAgreement (POST v1)
// ============================================================================

void SteamMicroTxn::processAgreement(quint64 orderId, quint64 steamId, quint64 agreementId,
                                      quint32 appId, qint64 amount, const QString &currency)
{
    if (!isValid()) {
        SteamProcessAgreementResponse errorResp;
        errorResp.success = false;
        errorResp.errorCode = QStringLiteral("INVALID_API_KEY");
        errorResp.errorDesc = QStringLiteral("API key is not set or empty");
        emit processAgreementFinished(errorResp);
        return;
    }

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("key"), m_apiKey);
    params.addQueryItem(QStringLiteral("orderid"), QString::number(orderId));
    params.addQueryItem(QStringLiteral("steamid"), QString::number(steamId));
    params.addQueryItem(QStringLiteral("agreementid"), QString::number(agreementId));
    params.addQueryItem(QStringLiteral("appid"), QString::number(appId));
    params.addQueryItem(QStringLiteral("amount"), QString::number(amount));
    params.addQueryItem(QStringLiteral("currency"), currency);

    sendPostRequest(QStringLiteral("ProcessAgreement/v1/"), params);
    m_currentEndpoint = QStringLiteral("ProcessAgreement");
}

// ============================================================================
// Private Network Methods
// ============================================================================

void SteamMicroTxn::sendGetRequest(const QString &endpoint, const QUrlQuery &params)
{
    // Abort any pending request
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }

    QUrl url(STEAM_API_BASE_URL + QStringLiteral("/") + endpoint);
    url.setQuery(params);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded"));
    request.setRawHeader("Accept", "application/json");

    qDebug() << "[SteamMicroTxn] GET:" << url.toDisplayString();

    m_currentReply = m_networkManager->get(request);
    setupReplyConnections();
}

void SteamMicroTxn::sendPostRequest(const QString &endpoint, const QUrlQuery &params)
{
    // Abort any pending request
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }

    QUrl url(STEAM_API_BASE_URL + QStringLiteral("/") + endpoint);
    QByteArray postData = params.query(QUrl::FullyEncoded).toUtf8();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded"));
    request.setRawHeader("Accept", "application/json");

    qDebug() << "[SteamMicroTxn] POST:" << url.toDisplayString();

    m_currentReply = m_networkManager->post(request, postData);
    setupReplyConnections();
}

void SteamMicroTxn::setupReplyConnections()
{
    if (!m_currentReply) {
        return;
    }

    connect(m_currentReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            [this](QNetworkReply::NetworkError error) {
                emit networkError(static_cast<int>(error),
                                  m_currentReply ? m_currentReply->errorString() : QString());
            });

    connect(m_currentReply, &QNetworkReply::sslErrors,
            [this](const QList<QSslError> &errors) {
                for (const QSslError &error : errors) {
                    qWarning() << "[SteamMicroTxn] SSL Error:" << error.errorString();
                }
            });
}

// ============================================================================
// Private Slots
// ============================================================================

void SteamMicroTxn::onReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }

    handleResponse(reply, m_currentEndpoint);
    reply->deleteLater();
    m_currentReply = nullptr;
    m_currentEndpoint.clear();
}

void SteamMicroTxn::handleResponse(QNetworkReply *reply, const QString &expectedEndpoint)
{
    // Check HTTP status code
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode != 200) {
        QString statusReason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
        qWarning() << "[SteamMicroTxn] HTTP Error:" << statusCode << statusReason;
        emit httpError(statusCode, statusReason);

        // Emit error for the appropriate endpoint
        if (expectedEndpoint == QStringLiteral("GetUserInfo")) {
            SteamUserInfo errorInfo;
            errorInfo.success = false;
            errorInfo.errorCode = QString::number(statusCode);
            errorInfo.errorDesc = statusReason;
            emit getUserInfoFinished(errorInfo);
        }
        return;
    }

    // Read response data
    QByteArray responseData = reply->readAll();
    qDebug() << "[SteamMicroTxn] Response:" << responseData;

    // Parse JSON
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "[SteamMicroTxn] JSON Parse Error:" << parseError.errorString();
        return;
    }

    // Route to appropriate parser
    if (expectedEndpoint == QStringLiteral("GetUserInfo")) {
        emit getUserInfoFinished(parseGetUserInfoResponse(jsonDoc));
    } else if (expectedEndpoint == QStringLiteral("InitTxn")) {
        emit initTxnFinished(parseInitTxnResponse(jsonDoc));
    } else if (expectedEndpoint == QStringLiteral("FinalizeTxn")) {
        emit finalizeTxnFinished(parseFinalizeTxnResponse(jsonDoc));
    } else if (expectedEndpoint == QStringLiteral("QueryTxn")) {
        emit queryTxnFinished(parseQueryTxnResponse(jsonDoc));
    } else if (expectedEndpoint == QStringLiteral("RefundTxn")) {
        emit refundTxnFinished(parseRefundTxnResponse(jsonDoc));
    } else if (expectedEndpoint == QStringLiteral("GetReport")) {
        emit getReportFinished(parseGetReportResponse(jsonDoc));
    } else if (expectedEndpoint == QStringLiteral("GetUserAgreementInfo")) {
        emit getUserAgreementInfoFinished(parseGetUserAgreementInfoResponse(jsonDoc));
    } else if (expectedEndpoint == QStringLiteral("CancelAgreement")) {
        emit cancelAgreementFinished(parseCancelAgreementResponse(jsonDoc));
    } else if (expectedEndpoint == QStringLiteral("AdjustAgreement")) {
        emit adjustAgreementFinished(parseAdjustAgreementResponse(jsonDoc));
    } else if (expectedEndpoint == QStringLiteral("ProcessAgreement")) {
        emit processAgreementFinished(parseProcessAgreementResponse(jsonDoc));
    }
}

// ============================================================================
// Response Parsing Methods
// ============================================================================

bool SteamMicroTxn::parseSuccess(const QJsonObject &response) const
{
    return response.value(QStringLiteral("result")).toString().toUpper() == QStringLiteral("OK");
}

QString SteamMicroTxn::parseErrorDesc(const QJsonObject &response) const
{
    QJsonObject error = response.value(QStringLiteral("error")).toObject();
    return error.value(QStringLiteral("errordesc")).toString();
}

QString SteamMicroTxn::parseErrorCode(const QJsonObject &response) const
{
    QJsonObject error = response.value(QStringLiteral("error")).toObject();
    return error.value(QStringLiteral("errorcode")).toString();
}

SteamUserInfo SteamMicroTxn::parseGetUserInfoResponse(const QJsonDocument &doc) const
{
    SteamUserInfo userInfo;
    QJsonObject response = doc.object().value(QStringLiteral("response")).toObject();

    if (!parseSuccess(response)) {
        userInfo.success = false;
        userInfo.errorCode = parseErrorCode(response);
        userInfo.errorDesc = parseErrorDesc(response);
        return userInfo;
    }

    userInfo.success = true;
    QJsonObject params = response.value(QStringLiteral("params")).toObject();

    userInfo.state = params.value(QStringLiteral("state")).toString();
    userInfo.country = params.value(QStringLiteral("country")).toString();
    userInfo.currency = params.value(QStringLiteral("currency")).toString();
    userInfo.status = parseAccountStatus(params.value(QStringLiteral("status")).toString());

    return userInfo;
}

SteamInitTxnResponse SteamMicroTxn::parseInitTxnResponse(const QJsonDocument &doc) const
{
    SteamInitTxnResponse resp;
    QJsonObject response = doc.object().value(QStringLiteral("response")).toObject();

    if (!parseSuccess(response)) {
        resp.success = false;
        resp.errorCode = parseErrorCode(response);
        resp.errorDesc = parseErrorDesc(response);
        return resp;
    }

    resp.success = true;
    QJsonObject params = response.value(QStringLiteral("params")).toObject();

    resp.orderId = params.value(QStringLiteral("orderid")).toString();
    resp.transId = params.value(QStringLiteral("transid")).toString();
    resp.steamUrl = params.value(QStringLiteral("steamurl")).toString();

    // Parse agreements array
    QJsonArray agreementsArray = params.value(QStringLiteral("agreements")).toArray();
    for (const QJsonValue &val : agreementsArray) {
        resp.agreements.append(val.toString());
    }

    return resp;
}

SteamFinalizeTxnResponse SteamMicroTxn::parseFinalizeTxnResponse(const QJsonDocument &doc) const
{
    SteamFinalizeTxnResponse resp;
    QJsonObject response = doc.object().value(QStringLiteral("response")).toObject();

    if (!parseSuccess(response)) {
        resp.success = false;
        resp.errorCode = parseErrorCode(response);
        resp.errorDesc = parseErrorDesc(response);
        return resp;
    }

    resp.success = true;
    QJsonObject params = response.value(QStringLiteral("params")).toObject();

    resp.orderId = params.value(QStringLiteral("orderid")).toString();
    resp.transId = params.value(QStringLiteral("transid")).toString();

    return resp;
}

SteamTransactionItem SteamMicroTxn::parseTransactionItem(const QJsonObject &obj) const
{
    SteamTransactionItem item;
    item.itemId = obj.value(QStringLiteral("itemid")).toVariant().toUInt();
    item.qty = obj.value(QStringLiteral("qty")).toVariant().toInt();
    item.amount = obj.value(QStringLiteral("amount")).toVariant().toLongLong();
    item.vat = obj.value(QStringLiteral("vat")).toVariant().toLongLong();
    item.status = parseItemStatus(obj.value(QStringLiteral("itemstatus")).toString());
    return item;
}

SteamQueryTxnResponse SteamMicroTxn::parseQueryTxnResponse(const QJsonDocument &doc) const
{
    SteamQueryTxnResponse resp;
    QJsonObject response = doc.object().value(QStringLiteral("response")).toObject();

    if (!parseSuccess(response)) {
        resp.success = false;
        resp.errorCode = parseErrorCode(response);
        resp.errorDesc = parseErrorDesc(response);
        return resp;
    }

    resp.success = true;
    QJsonObject params = response.value(QStringLiteral("params")).toObject();

    resp.orderId = params.value(QStringLiteral("orderid")).toString();
    resp.transId = params.value(QStringLiteral("transid")).toString();
    resp.steamId = params.value(QStringLiteral("steamid")).toString();
    resp.status = parseTransactionStatus(params.value(QStringLiteral("status")).toString());
    resp.currency = params.value(QStringLiteral("currency")).toString();
    resp.time = params.value(QStringLiteral("time")).toString();

    // Parse items array
    QJsonArray itemsArray = params.value(QStringLiteral("items")).toArray();
    for (const QJsonValue &val : itemsArray) {
        resp.items.append(parseTransactionItem(val.toObject()));
    }

    return resp;
}

SteamRefundTxnResponse SteamMicroTxn::parseRefundTxnResponse(const QJsonDocument &doc) const
{
    SteamRefundTxnResponse resp;
    QJsonObject response = doc.object().value(QStringLiteral("response")).toObject();

    if (!parseSuccess(response)) {
        resp.success = false;
        resp.errorCode = parseErrorCode(response);
        resp.errorDesc = parseErrorDesc(response);
        return resp;
    }

    resp.success = true;
    QJsonObject params = response.value(QStringLiteral("params")).toObject();

    resp.orderId = params.value(QStringLiteral("orderid")).toString();
    resp.transId = params.value(QStringLiteral("transid")).toString();

    return resp;
}

SteamGetReportResponse SteamMicroTxn::parseGetReportResponse(const QJsonDocument &doc) const
{
    SteamGetReportResponse resp;
    QJsonObject response = doc.object().value(QStringLiteral("response")).toObject();

    if (!parseSuccess(response)) {
        resp.success = false;
        resp.errorCode = parseErrorCode(response);
        resp.errorDesc = parseErrorDesc(response);
        return resp;
    }

    resp.success = true;
    QJsonObject params = response.value(QStringLiteral("params")).toObject();

    resp.count = params.value(QStringLiteral("count")).toInt();

    // Parse orders array
    QJsonArray ordersArray = params.value(QStringLiteral("orders")).toArray();
    for (const QJsonValue &val : ordersArray) {
        QJsonObject orderObj = val.toObject();
        SteamReportOrder order;
        order.orderId = orderObj.value(QStringLiteral("orderid")).toString();
        order.transId = orderObj.value(QStringLiteral("transid")).toString();
        order.steamId = orderObj.value(QStringLiteral("steamid")).toString();
        order.status = parseTransactionStatus(orderObj.value(QStringLiteral("status")).toString());
        order.currency = orderObj.value(QStringLiteral("currency")).toString();
        order.time = orderObj.value(QStringLiteral("time")).toString();
        order.country = orderObj.value(QStringLiteral("country")).toString();
        order.usState = orderObj.value(QStringLiteral("usstate")).toString();
        order.timeCreated = orderObj.value(QStringLiteral("timecreated")).toString();

        // Parse items
        QJsonArray itemsArray = orderObj.value(QStringLiteral("items")).toArray();
        for (const QJsonValue &itemVal : itemsArray) {
            order.items.append(parseTransactionItem(itemVal.toObject()));
        }

        resp.orders.append(order);
    }

    return resp;
}

SteamAgreementInfo SteamMicroTxn::parseAgreementInfo(const QJsonObject &obj) const
{
    SteamAgreementInfo info;
    info.agreementId = obj.value(QStringLiteral("agreementid")).toString();
    info.itemId = obj.value(QStringLiteral("itemid")).toString();
    info.status = parseAgreementStatus(obj.value(QStringLiteral("status")).toString());
    info.period = obj.value(QStringLiteral("period")).toString();
    info.frequency = obj.value(QStringLiteral("frequency")).toVariant().toUInt();
    info.startDate = obj.value(QStringLiteral("startdate")).toString();
    info.endDate = obj.value(QStringLiteral("enddate")).toString();
    info.recurringAmt = obj.value(QStringLiteral("recurringamt")).toVariant().toLongLong();
    info.currency = obj.value(QStringLiteral("currency")).toString();
    info.timeCreated = obj.value(QStringLiteral("timecreated")).toString();
    info.lastPayment = obj.value(QStringLiteral("lastpayment")).toString();
    info.lastAmount = obj.value(QStringLiteral("lastamount")).toVariant().toLongLong();
    info.nextPayment = obj.value(QStringLiteral("nextpayment")).toString();
    info.outstanding = obj.value(QStringLiteral("outstanding")).toVariant().toLongLong();
    info.failedAttempts = obj.value(QStringLiteral("failedattempts")).toVariant().toUInt();
    return info;
}

SteamGetUserAgreementInfoResponse SteamMicroTxn::parseGetUserAgreementInfoResponse(const QJsonDocument &doc) const
{
    SteamGetUserAgreementInfoResponse resp;
    QJsonObject response = doc.object().value(QStringLiteral("response")).toObject();

    if (!parseSuccess(response)) {
        resp.success = false;
        resp.errorCode = parseErrorCode(response);
        resp.errorDesc = parseErrorDesc(response);
        return resp;
    }

    resp.success = true;
    QJsonObject params = response.value(QStringLiteral("params")).toObject();

    // Parse agreements
    QJsonObject agreementsObj = params.value(QStringLiteral("agreements")).toObject();
    QJsonObject agreementObj = agreementsObj.value(QStringLiteral("agreement")).toObject();
    resp.agreements.append(parseAgreementInfo(agreementObj));

    return resp;
}

SteamCancelAgreementResponse SteamMicroTxn::parseCancelAgreementResponse(const QJsonDocument &doc) const
{
    SteamCancelAgreementResponse resp;
    QJsonObject response = doc.object().value(QStringLiteral("response")).toObject();

    if (!parseSuccess(response)) {
        resp.success = false;
        resp.errorCode = parseErrorCode(response);
        resp.errorDesc = parseErrorDesc(response);
        return resp;
    }

    resp.success = true;
    QJsonObject params = response.value(QStringLiteral("params")).toObject();
    resp.agreementId = params.value(QStringLiteral("agreementid")).toString();

    return resp;
}

SteamAdjustAgreementResponse SteamMicroTxn::parseAdjustAgreementResponse(const QJsonDocument &doc) const
{
    SteamAdjustAgreementResponse resp;
    QJsonObject response = doc.object().value(QStringLiteral("response")).toObject();

    if (!parseSuccess(response)) {
        resp.success = false;
        resp.errorCode = parseErrorCode(response);
        resp.errorDesc = parseErrorDesc(response);
        return resp;
    }

    resp.success = true;
    QJsonObject params = response.value(QStringLiteral("params")).toObject();
    resp.agreementId = params.value(QStringLiteral("agreementid")).toString();
    resp.nextProcessDate = params.value(QStringLiteral("nextprocessdate")).toString();

    return resp;
}

SteamProcessAgreementResponse SteamMicroTxn::parseProcessAgreementResponse(const QJsonDocument &doc) const
{
    SteamProcessAgreementResponse resp;
    QJsonObject response = doc.object().value(QStringLiteral("response")).toObject();

    if (!parseSuccess(response)) {
        resp.success = false;
        resp.errorCode = parseErrorCode(response);
        resp.errorDesc = parseErrorDesc(response);
        return resp;
    }

    resp.success = true;
    QJsonObject params = response.value(QStringLiteral("params")).toObject();
    resp.orderId = params.value(QStringLiteral("orderid")).toString();
    resp.transId = params.value(QStringLiteral("transid")).toString();
    resp.agreementId = params.value(QStringLiteral("agreementid")).toString();

    return resp;
}

// ============================================================================
// Helper Methods
// ============================================================================

SteamAccountStatus SteamMicroTxn::parseAccountStatus(const QString &statusString) const
{
    if (statusString.isEmpty()) {
        return SteamAccountStatus::Unknown;
    }
    QString lower = statusString.toLower();
    if (lower.contains(QStringLiteral("locked"))) return SteamAccountStatus::Locked;
    if (lower.contains(QStringLiteral("trusted"))) return SteamAccountStatus::Trusted;
    if (lower.contains(QStringLiteral("active"))) return SteamAccountStatus::Active;
    return SteamAccountStatus::Unknown;
}

SteamTransactionStatus SteamMicroTxn::parseTransactionStatus(const QString &statusString) const
{
    if (statusString.isEmpty()) {
        return SteamTransactionStatus::Unknown;
    }
    QString lower = statusString.toLower();
    if (lower == QStringLiteral("succeeded")) return SteamTransactionStatus::Succeeded;
    if (lower == QStringLiteral("failed")) return SteamTransactionStatus::Failed;
    if (lower == QStringLiteral("refunded")) return SteamTransactionStatus::Refunded;
    if (lower == QStringLiteral("partialrefund")) return SteamTransactionStatus::PartialRefund;
    if (lower == QStringLiteral("reversed")) return SteamTransactionStatus::Reversed;
    if (lower == QStringLiteral("pending")) return SteamTransactionStatus::Pending;
    return SteamTransactionStatus::Unknown;
}

SteamItemStatus SteamMicroTxn::parseItemStatus(const QString &statusString) const
{
    if (statusString.isEmpty()) {
        return SteamItemStatus::Unknown;
    }
    QString lower = statusString.toLower();
    if (lower == QStringLiteral("succeeded")) return SteamItemStatus::Succeeded;
    if (lower == QStringLiteral("failed")) return SteamItemStatus::Failed;
    if (lower == QStringLiteral("refunded")) return SteamItemStatus::Refunded;
    if (lower == QStringLiteral("pending")) return SteamItemStatus::Pending;
    return SteamItemStatus::Unknown;
}

SteamAgreementStatus SteamMicroTxn::parseAgreementStatus(const QString &statusString) const
{
    if (statusString.isEmpty()) {
        return SteamAgreementStatus::Unknown;
    }
    QString lower = statusString.toLower();
    if (lower == QStringLiteral("active")) return SteamAgreementStatus::Active;
    if (lower == QStringLiteral("canceled")) return SteamAgreementStatus::Canceled;
    if (lower == QStringLiteral("processing")) return SteamAgreementStatus::Processing;
    if (lower == QStringLiteral("expired")) return SteamAgreementStatus::Expired;
    return SteamAgreementStatus::Unknown;
}

QString SteamMicroTxn::reportTypeToString(SteamReportType type) const
{
    switch (type) {
    case SteamReportType::GameSales: return QStringLiteral("GAMESALES");
    case SteamReportType::SteamStoreSales: return QStringLiteral("STEAMSTORESALES");
    case SteamReportType::Settlement: return QStringLiteral("SETTLEMENT");
    case SteamReportType::Chargeback: return QStringLiteral("CHARGEBACK");
    case SteamReportType::Subscription: return QStringLiteral("SUBSCRIPTION");
    default: return QStringLiteral("GAMESALES");
    }
}
