/**
 * @file steam_microtxn.h
 * @brief Steam ISteamMicroTxn API wrapper for Qt/C++
 *
 * Provides functionality to interact with Steam's Microtransaction API.
 * Implements all endpoints: InitTxn, GetUserInfo, FinalizeTxn, QueryTxn,
 * RefundTxn, GetReport, CancelAgreement, AdjustAgreement, ProcessAgreement,
 * GetUserAgreementInfo.
 *
 * @author MiniMax Agent
 * @date 2026-03-28
 */

#ifndef STEAM_MICROTXN_H
#define STEAM_MICROTXN_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QString>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <QByteArray>
#include <QDebug>
#include <QUrlQuery>

// ============================================================================
// Forward Declarations
// ============================================================================

class SteamMicroTxn;

// ============================================================================
// Enumerations
// ============================================================================

/**
 * @brief Account status enumeration for Steam user purchasing capability
 */
enum class SteamAccountStatus {
    Locked,   ///< Cannot make purchases
    Active,   ///< Default state, can make purchases
    Trusted,  ///< Enhanced trust status (purchase >90 days without chargeback)
    Unknown   ///< Status not determined
};

/**
 * @brief Transaction status enumeration
 */
enum class SteamTransactionStatus {
    Succeeded,
    Failed,
    Refunded,
    PartialRefund,
    Reversed,
    Pending,
    Unknown
};

/**
 * @brief Item status enumeration
 */
enum class SteamItemStatus {
    Succeeded,
    Failed,
    Refunded,
    Pending,
    Unknown
};

/**
 * @brief Agreement status enumeration
 */
enum class SteamAgreementStatus {
    Active,
    Canceled,
    Processing,
    Expired,
    Unknown
};

/**
 * @brief Report type enumeration
 */
enum class SteamReportType {
    GameSales,
    SteamStoreSales,
    Settlement,
    Chargeback,
    Subscription
};

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief Response structure for GetUserInfo API call
 */
struct SteamUserInfo {
    QString state;                   ///< US State. Empty for non-US countries.
    QString country;                  ///< ISO 3166-1-alpha-2 country code.
    QString currency;                 ///< ISO 4217 currency code of prices.
    SteamAccountStatus status;        ///< Account status for purchasing.
    bool success = false;            ///< Whether the API call was successful.
    QString errorCode;                ///< Error code if request failed.
    QString errorDesc;                ///< Error description if request failed.
};

/**
 * @brief Item in a transaction
 */
struct SteamTransactionItem {
    quint32 itemId = 0;              ///< 3rd party ID for item
    qint16 qty = 0;                  ///< Quantity of this item
    qint64 amount = 0;               ///< Total cost in cents
    qint64 vat = 0;                  ///< VAT amount in cents
    SteamItemStatus status = SteamItemStatus::Unknown;  ///< Item status
    QString description;              ///< Item description
    QString category;                ///< Item category
    QString billingType;             ///< Billing type (Steam/Game)
    QString period;                  ///< Recurring period
    quint32 frequency = 0;           ///< Frequency in days
    qint64 recurringAmt = 0;        ///< Recurring billing amount
    quint32 associatedBundle = 0;   ///< Associated bundle ID
};

/**
 * @brief Bundle in a transaction
 */
struct SteamTransactionBundle {
    quint32 bundleId = 0;            ///< 3rd party ID of bundle
    quint32 qty = 0;                 ///< Quantity of bundle
    QString description;             ///< Bundle description
    QString category;                ///< Bundle category
};

/**
 * @brief Response structure for InitTxn API call
 */
struct SteamInitTxnResponse {
    QString orderId;                 ///< Unique 64-bit ID for order
    QString transId;                 ///< Steam transaction ID
    QString steamUrl;                ///< URL for web session authorization
    QVector<QString> agreements;    ///< List of billing agreement IDs
    bool success = false;            ///< Whether the API call was successful.
    QString errorCode;               ///< Error code if request failed.
    QString errorDesc;                ///< Error description if request failed.
};

/**
 * @brief Response structure for FinalizeTxn API call
 */
struct SteamFinalizeTxnResponse {
    QString orderId;                 ///< Unique 64-bit ID for order
    QString transId;                 ///< Steam transaction ID
    bool success = false;            ///< Whether the API call was successful.
    QString errorCode;               ///< Error code if request failed.
    QString errorDesc;                ///< Error description if request failed.
};

/**
 * @brief Response structure for QueryTxn API call
 */
struct SteamQueryTxnResponse {
    QString orderId;                 ///< Unique 64-bit ID for order
    QString transId;                 ///< Steam transaction ID
    QString steamId;                 ///< Steam ID of user
    SteamTransactionStatus status = SteamTransactionStatus::Unknown;  ///< Transaction status
    QString currency;                 ///< Currency code
    QString time;                     ///< Transaction time (RFC 3339)
    QVector<SteamTransactionItem> items;  ///< Items in the transaction
    bool success = false;            ///< Whether the API call was successful.
    QString errorCode;               ///< Error code if request failed.
    QString errorDesc;                ///< Error description if request failed.
};

/**
 * @brief Response structure for RefundTxn API call
 */
struct SteamRefundTxnResponse {
    QString orderId;                 ///< Unique 64-bit ID for order
    QString transId;                 ///< Steam transaction ID
    bool success = false;            ///< Whether the API call was successful.
    QString errorCode;               ///< Error code if request failed.
    QString errorDesc;                ///< Error description if request failed.
};

/**
 * @brief Order information in a report
 */
struct SteamReportOrder {
    QString orderId;                 ///< Unique 64-bit ID for order
    QString transId;                 ///< Steam transaction ID
    QString steamId;                 ///< Steam ID of user
    SteamTransactionStatus status = SteamTransactionStatus::Unknown;  ///< Order status
    QString currency;                 ///< Currency code
    QString time;                     ///< Transaction time
    QString country;                  ///< Country code
    QString usState;                  ///< US state (if applicable)
    QString timeCreated;              ///< Order creation time
    QVector<SteamTransactionItem> items;  ///< Items in the order
};

/**
 * @brief Response structure for GetReport API call
 */
struct SteamGetReportResponse {
    int count = 0;                   ///< Number of orders in report
    QVector<SteamReportOrder> orders; ///< Orders in the report
    bool success = false;            ///< Whether the API call was successful.
    QString errorCode;               ///< Error code if request failed.
    QString errorDesc;                ///< Error description if request failed.
};

/**
 * @brief Billing agreement information
 */
struct SteamAgreementInfo {
    QString agreementId;             ///< Unique 64-bit Steam billing agreement ID
    QString itemId;                  ///< Associated item ID
    SteamAgreementStatus status = SteamAgreementStatus::Unknown;  ///< Agreement status
    QString period;                   ///< Billing period
    quint32 frequency = 0;          ///< Billing frequency
    QString startDate;                ///< Agreement start date
    QString endDate;                 ///< Agreement end date
    qint64 recurringAmt = 0;        ///< Recurring billing amount
    QString currency;                 ///< Currency code
    QString timeCreated;             ///< Creation time
    QString lastPayment;             ///< Last payment time
    qint64 lastAmount = 0;          ///< Last payment amount
    QString nextPayment;             ///< Next scheduled payment
    qint64 outstanding = 0;         ///< Outstanding balance
    quint32 failedAttempts = 0;      ///< Failed payment attempts
};

/**
 * @brief Response structure for GetUserAgreementInfo API call
 */
struct SteamGetUserAgreementInfoResponse {
    QVector<SteamAgreementInfo> agreements;  ///< User's billing agreements
    bool success = false;            ///< Whether the API call was successful.
    QString errorCode;               ///< Error code if request failed.
    QString errorDesc;                ///< Error description if request failed.
};

/**
 * @brief Response structure for CancelAgreement API call
 */
struct SteamCancelAgreementResponse {
    QString agreementId;             ///< Cancelled agreement ID
    bool success = false;            ///< Whether the API call was successful.
    QString errorCode;               ///< Error code if request failed.
    QString errorDesc;                ///< Error description if request failed.
};

/**
 * @brief Response structure for AdjustAgreement API call
 */
struct SteamAdjustAgreementResponse {
    QString agreementId;             ///< Adjusted agreement ID
    QString nextProcessDate;         ///< New next payment date
    bool success = false;            ///< Whether the API call was successful.
    QString errorCode;               ///< Error code if request failed.
    QString errorDesc;                ///< Error description if request failed.
};

/**
 * @brief Response structure for ProcessAgreement API call
 */
struct SteamProcessAgreementResponse {
    QString orderId;                 ///< Order ID for this payment
    QString transId;                 ///< Steam transaction ID
    QString agreementId;            ///< Agreement ID processed
    bool success = false;            ///< Whether the API call was successful.
    QString errorCode;               ///< Error code if request failed.
    QString errorDesc;                ///< Error description if request failed.
};

/**
 * @brief Item parameters for InitTxn
 */
struct SteamItemParams {
    quint32 itemId = 0;             ///< 3rd party ID for item
    qint16 qty = 1;                 ///< Quantity (default 1)
    qint64 amount = 0;              ///< Cost in cents (required)
    QString description;             ///< Item description (max 128 chars)
    QString category;               ///< Category (max 64 chars)
    QString billingType;            ///< "Steam" or "Game" for recurring
    QString period;                 ///< "Day", "Week", "Month", "Year"
    quint32 frequency = 0;          ///< Frequency in days (1-255)
    qint64 recurringAmt = 0;        ///< Future recurring billing amount
    quint32 associatedBundle = 0;   ///< Associated bundle ID
};

/**
 * @brief Bundle parameters for InitTxn
 */
struct SteamBundleParams {
    quint32 bundleId = 0;           ///< 3rd party bundle ID
    quint32 qty = 1;                ///< Quantity (default 1)
    QString description;            ///< Bundle description (max 128 chars)
    QString category;               ///< Category (max 64 chars)
};

// ============================================================================
// SteamMicroTxn Class
// ============================================================================

/**
 * @brief SteamMicroTxn class provides interface to Steam's Microtransaction API
 *
 * This class wraps the ISteamMicroTxn Web API for Qt applications, providing
 * a convenient way to manage microtransactions, recurring billing, and reports.
 *
 * Example usage:
 * @code
 * SteamMicroTxn *microTxn = new SteamMicroTxn("YOUR_API_KEY", this);
 *
 * // Get user info
 * connect(microTxn, &SteamMicroTxn::getUserInfoFinished, [](const SteamUserInfo &info) {
 *     qDebug() << "Country:" << info.country << "Currency:" << info.currency;
 * });
 * microTxn->getUserInfo(12345, 76561198000000000);
 *
 * // Initialize transaction
 * connect(microTxn, &SteamMicroTxn::initTxnFinished, [](const SteamInitTxnResponse &resp) {
 *     qDebug() << "Order ID:" << resp.orderId << "Trans ID:" << resp.transId;
 * });
 * microTxn->initTxn(...);
 * @endcode
 */
class SteamMicroTxn : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Construct a SteamMicroTxn instance
     * @param apiKey Steamworks Web API publisher key with Microtransaction permissions
     * @param parent Optional parent QObject
     */
    explicit SteamMicroTxn(const QString &apiKey, QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~SteamMicroTxn();

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Set the API key
     * @param apiKey Steamworks Web API publisher key
     */
    void setApiKey(const QString &apiKey);

    /**
     * @brief Get the current API key
     * @return Current API key string
     */
    QString apiKey() const;

    /**
     * @brief Check if the API key is set
     * @return true if API key is not empty
     */
    bool isValid() const;

    /**
     * @brief Get the base URL for API requests
     * @return Base URL string
     */
    static QString baseUrl();

    // ========================================================================
    // GetUserInfo (GET v2)
    // ========================================================================

    /**
     * @brief Get user purchasing information
     * @param appId AppID of the game
     * @param steamId Steam ID of the user (optional, pass 0)
     * @param ipAddress User IP address (required if usersession was "web")
     *
     * Retrieves details for a user's purchasing info based upon their Steam wallet.
     */
    void getUserInfo(quint64 appId, quint64 steamId = 0, const QString &ipAddress = QString());

    // ========================================================================
    // InitTxn (POST v3)
    // ========================================================================

    /**
     * @brief Initialize a transaction
     * @param orderId Unique 64-bit order ID (must be unique per order)
     * @param steamId Steam ID of the user
     * @param appId App ID of the game
     * @param items Vector of item parameters
     * @param language ISO 639-1 language code for item descriptions
     * @param currency ISO 4217 currency code
     * @param userSession Session type ("client" or "web")
     * @param ipAddress User IP address (required if userSession is "web")
     * @param bundles Optional vector of bundle parameters
     *
     * Initializes a purchase transaction. After authorization, call finalizeTxn().
     */
    void initTxn(quint64 orderId, quint64 steamId, quint32 appId,
                 const QVector<SteamItemParams> &items,
                 const QString &language, const QString &currency,
                 const QString &userSession = QStringLiteral("client"),
                 const QString &ipAddress = QString(),
                 const QVector<SteamBundleParams> &bundles = QVector<SteamBundleParams>());

    // ========================================================================
    // FinalizeTxn (POST v2)
    // ========================================================================

    /**
     * @brief Finalize a transaction
     * @param orderId Unique 64-bit order ID
     * @param appId App ID of the game
     *
     * Finalizes a transaction after user authorization.
     * Must be called after InitTxn when userSession is "web".
     */
    void finalizeTxn(quint64 orderId, quint32 appId);

    // ========================================================================
    // QueryTxn (GET v3)
    // ========================================================================

    /**
     * @brief Query transaction status
     * @param appId App ID of the game
     * @param orderId Order ID (pass 0 if using transId)
     * @param transId Transaction ID (pass 0 if using orderId)
     *
     * Query the status of a specific transaction.
     * Either orderId or transId must be provided.
     */
    void queryTxn(quint32 appId, quint64 orderId = 0, quint64 transId = 0);

    // ========================================================================
    // RefundTxn (POST v2)
    // ========================================================================

    /**
     * @brief Refund a transaction
     * @param orderId Unique 64-bit order ID to refund
     * @param appId App ID of the game
     *
     * Refunds a transaction. The refund will be processed immediately.
     */
    void refundTxn(quint64 orderId, quint32 appId);

    // ========================================================================
    // GetReport (GET v5)
    // ========================================================================

    /**
     * @brief Get sales/settlement report
     * @param appId App ID of the game
     * @param startTime Report start time (RFC 3339 UTC format)
     * @param type Report type (GameSales, SteamStoreSales, Settlement, etc.)
     * @param maxResults Maximum results to return (default 1000, max 50000)
     *
     * Retrieves a report of transactions within the specified time range.
     */
    void getReport(quint32 appId, const QString &startTime,
                   SteamReportType type = SteamReportType::GameSales,
                   quint32 maxResults = 1000);

    // ========================================================================
    // GetUserAgreementInfo (GET v2)
    // ========================================================================

    /**
     * @brief Get user's billing agreement information
     * @param steamId Steam ID of the user
     * @param appId App ID of the game
     *
     * Retrieves information about all active billing agreements for a user.
     */
    void getUserAgreementInfo(quint64 steamId, quint32 appId);

    // ========================================================================
    // CancelAgreement (POST v1)
    // ========================================================================

    /**
     * @brief Cancel a billing agreement
     * @param steamId Steam ID of the user
     * @param agreementId Billing agreement ID to cancel
     * @param appId App ID of the game
     *
     * Cancels an active recurring billing agreement.
     */
    void cancelAgreement(quint64 steamId, quint64 agreementId, quint32 appId);

    // ========================================================================
    // AdjustAgreement (POST v1)
    // ========================================================================

    /**
     * @brief Adjust a billing agreement
     * @param steamId Steam ID of the user
     * @param agreementId Billing agreement ID to adjust
     * @param appId App ID of the game
     * @param nextProcessDate New next payment date (YYYYMMDD format)
     * @param oldNextProcessDate Last known payment date for verification (optional)
     *
     * Adjusts the next payment date for a recurring billing agreement.
     * The date can only be adjusted forward, not backward.
     */
    void adjustAgreement(quint64 steamId, quint64 agreementId, quint32 appId,
                         const QString &nextProcessDate,
                         const QString &oldNextProcessDate = QString());

    // ========================================================================
    // ProcessAgreement (POST v1)
    // ========================================================================

    /**
     * @brief Process a recurring billing payment
     * @param orderId Unique 64-bit order ID (0 if from Steam store)
     * @param steamId Steam ID of the user
     * @param agreementId Billing agreement ID
     * @param appId App ID of the game
     * @param amount Total cost in cents
     * @param currency ISO 4217 currency code
     *
     * Processes a payment for an existing billing agreement.
     */
    void processAgreement(quint64 orderId, quint64 steamId, quint64 agreementId,
                          quint32 appId, qint64 amount, const QString &currency);

    // ========================================================================
    // Signals
    // ========================================================================

signals:
    /**
     * @brief Emitted when GetUserInfo request completes
     */
    void getUserInfoFinished(const SteamUserInfo &userInfo);

    /**
     * @brief Emitted when InitTxn request completes
     */
    void initTxnFinished(const SteamInitTxnResponse &response);

    /**
     * @brief Emitted when FinalizeTxn request completes
     */
    void finalizeTxnFinished(const SteamFinalizeTxnResponse &response);

    /**
     * @brief Emitted when QueryTxn request completes
     */
    void queryTxnFinished(const SteamQueryTxnResponse &response);

    /**
     * @brief Emitted when RefundTxn request completes
     */
    void refundTxnFinished(const SteamRefundTxnResponse &response);

    /**
     * @brief Emitted when GetReport request completes
     */
    void getReportFinished(const SteamGetReportResponse &response);

    /**
     * @brief Emitted when GetUserAgreementInfo request completes
     */
    void getUserAgreementInfoFinished(const SteamGetUserAgreementInfoResponse &response);

    /**
     * @brief Emitted when CancelAgreement request completes
     */
    void cancelAgreementFinished(const SteamCancelAgreementResponse &response);

    /**
     * @brief Emitted when AdjustAgreement request completes
     */
    void adjustAgreementFinished(const SteamAdjustAgreementResponse &response);

    /**
     * @brief Emitted when ProcessAgreement request completes
     */
    void processAgreementFinished(const SteamProcessAgreementResponse &response);

    /**
     * @brief Emitted when a network error occurs
     */
    void networkError(int errorCode, const QString &errorString);

    /**
     * @brief Emitted when an HTTP error occurs
     */
    void httpError(int statusCode, const QString &statusReason);

private slots:
    void onReplyFinished();

private:
    // ========================================================================
    // Private Helpers
    // ========================================================================

    void sendRequest(const QString &endpoint, const QString &method,
                     const QUrl &url, const QByteArray &postData = QByteArray());

    void sendPostRequest(const QString &endpoint, const QUrlQuery &params);

    void sendGetRequest(const QString &endpoint, const QUrlQuery &params);

    void handleResponse(QNetworkReply *reply, const QString &expectedEndpoint);

    // Parsing methods
    SteamUserInfo parseGetUserInfoResponse(const QJsonDocument &doc) const;
    SteamInitTxnResponse parseInitTxnResponse(const QJsonDocument &doc) const;
    SteamFinalizeTxnResponse parseFinalizeTxnResponse(const QJsonDocument &doc) const;
    SteamQueryTxnResponse parseQueryTxnResponse(const QJsonDocument &doc) const;
    SteamRefundTxnResponse parseRefundTxnResponse(const QJsonDocument &doc) const;
    SteamGetReportResponse parseGetReportResponse(const QJsonDocument &doc) const;
    SteamGetUserAgreementInfoResponse parseGetUserAgreementInfoResponse(const QJsonDocument &doc) const;
    SteamCancelAgreementResponse parseCancelAgreementResponse(const QJsonDocument &doc) const;
    SteamAdjustAgreementResponse parseAdjustAgreementResponse(const QJsonDocument &doc) const;
    SteamProcessAgreementResponse parseProcessAgreementResponse(const QJsonDocument &doc) const;

    // Helper methods
    SteamAccountStatus parseAccountStatus(const QString &statusString) const;
    SteamTransactionStatus parseTransactionStatus(const QString &statusString) const;
    SteamItemStatus parseItemStatus(const QString &statusString) const;
    SteamAgreementStatus parseAgreementStatus(const QString &statusString) const;
    QString reportTypeToString(SteamReportType type) const;

    // ========================================================================
    // Private Members
    // ========================================================================

    QString m_apiKey;
    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentReply;
    QString m_currentEndpoint;
};

#endif // STEAM_MICROTXN_H
