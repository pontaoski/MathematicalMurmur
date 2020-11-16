#include <QCoreApplication>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>

#include "Util.hpp"

auto toNetworkResult(QNetworkReply *reply) -> NetworkResult
{
	auto ret = NetworkResult();
	defer(delete reply);

	if (reply->error() == QNetworkReply::NoError)
	{
		auto doc = QJsonDocument::fromJson(reply->readAll());
		ret.std::variant<NetworkError, QJsonDocument>::operator=(doc);
	}
	else
	{
		ret.std::variant<NetworkError, QJsonDocument>::operator=(NetworkError{
			.code = reply->error(),
			.desc = reply->errorString()});
	}

	return ret;
}

template <typename T>
auto post(const QUrl &url, const T &body) -> NetworkResult
{
	auto nam = new QNetworkAccessManager;
	defer(delete nam);

	QNetworkRequest req(url);

	return toNetworkResult(await(nam->post(req, body)));
}

auto get(const QUrl &url) -> NetworkResult
{
	auto nam = new QNetworkAccessManager;
	defer(delete nam);

	QNetworkRequest req(url);

	return toNetworkResult(await(nam->get(req)));
}

template <typename...> struct WhichType;

template <typename T>
constexpr auto checkForIsQList(const QList<T> &) { return true; }

template <typename T>
constexpr auto checkForIsQList(const T &) { return false; }

template <typename T>
auto unmarshal(const QJsonObject &obj) -> T;

template <typename T>
T unmarshalObject(const QJsonObject &obj)
{
	T t;
	constexpr auto count = MemberCounter<T>();

	if constexpr (count == 1)
	{
		auto &[first] = t;
		unmarshalMember(first, obj[first.tag]);
	}
	else if constexpr (count == 2)
	{
		auto &[first, second] = t;
		unmarshalMember(first, obj[first.tag]);
		unmarshalMember(second, obj[second.tag]);
	}
	else if constexpr (count == 3)
	{
		auto &[first, second, third] = t;
		unmarshalMember(first, obj[first.tag]);
		unmarshalMember(second, obj[second.tag]);
		unmarshalMember(third, obj[third.tag]);
	}
	else if constexpr (count == 4)
	{
		auto &[first, second, third, fourth] = t;
		unmarshalMember(first, obj[first.tag]);
		unmarshalMember(second, obj[second.tag]);
		unmarshalMember(third, obj[third.tag]);
		unmarshalMember(fourth, obj[fourth.tag]);
	}
	else
	{
		qWarning() << "too many members...";
	}

	return t;
}

template <typename T>
void unmarshalMember(T &member, const QJsonValue &val)
{
	using tagKind = T::kind;

	constexpr bool isString = std::is_convertible<tagKind, QString>::value;
	constexpr bool isBool = std::is_convertible<tagKind, bool>::value;
	constexpr bool isDouble = std::is_convertible<tagKind, double>::value;
	constexpr bool isInt = std::is_convertible<tagKind, int>::value;

	if constexpr (isString)
	{
		if (!val.isString())
		{
			qWarning() << "Expected string, got" << val.type();
		}
		member = val.toString();
	}
	else if constexpr (isBool)
	{
		if (!val.isBool())
		{
			qWarning() << "Expected bool, got" << val.type();
		}
		member = val.toBool();
	}
	else if constexpr (isDouble)
	{
		if (!val.isDouble())
		{
			qWarning() << "Expected number, got" << val.type();
		}
		member = val.toDouble();
	}
	else if constexpr (isInt)
	{
		if (!val.isDouble())
		{
			qWarning() << "Expected number, got" << val.type();
		}
		member = val.toInt();
	}
	else
	{
		constexpr bool isArray = requires(tagKind x)
		{
			x.detach();
			x.detachShared();
			x.isEmpty();
			x.clear();
			x.takeFirst();
		};

		if constexpr (isArray)
		{
			if (!val.isArray())
			{
				qWarning() << "Expected number, got array" << val.type();
			}

			auto array = val.toArray();
			tagKind t;
			using arrayType = decltype(t.takeFirst());

			for (auto item : array)
			{
				constexpr bool isMember = requires { arrayType::kind; arrayType::tag; };

				if constexpr (isMember) {
					arrayType yoot;
					unmarshalMember<arrayType>(yoot, item);
					t << yoot;
				} else {
					t << unmarshalObject<arrayType>(item.toObject());
				}
			}

			member = t;
		}
		else
		{
			member = unmarshalObject<tagKind>(val.toObject());
		}
	}
}

template <typename T>
auto unmarshal(const QJsonDocument &doc) -> T
{
	T t;

	if (doc.isObject()) {
		return unmarshalObject<T>(doc.object());
	}

	return t;
}

struct Flows
{
	struct FlowEntry {
		TaggedMember<"type",QString> name;
	};

	TaggedMember<"flows", QList<FlowEntry>> flows;
};

QUrl operator%(const QUrl &lhs, const QString &rhs)
{
	auto copy = lhs;
	copy.setPath(rhs);
	return copy;
}

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);

	auto homeserver = QUrl("https://matrix.tchncs.de");
	auto reply = get(homeserver % "/_matrix/client/r0/login");
	auto flows = unmarshal<Flows>(reply.value());

	return 0;
}
