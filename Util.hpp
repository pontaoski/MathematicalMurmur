#pragma once

#include <QScopeGuard>
#include <QJsonDocument>
#include <variant>

#define await(resp) [=]() { auto thing = resp; while (!thing->isFinished()) { QCoreApplication::processEvents(); }; return thing; }()
#define defer(data) auto a##__COUNTER__ = qScopeGuard([=] { data; })

struct NetworkError
{
	QNetworkReply::NetworkError code;
	QString desc;
};

struct NetworkResult : public std::variant<NetworkError, QJsonDocument>
{
	bool isError() const { return std::holds_alternative<NetworkError>(*this); }
	NetworkError error() const { return std::get<NetworkError>(*this); }
	bool isSuccess() const { return std::holds_alternative<QJsonDocument>(*this); }
	QJsonDocument value() const { return std::get<QJsonDocument>(*this); }
};

template <size_t N>
struct StringLiteral
{
	constexpr StringLiteral(const char (&str)[N])
	{
		std::copy_n(str, N, value);
	}

	char value[N];
};

template<StringLiteral Tag, typename T>
struct TaggedMember
{
	T val;

	TaggedMember() = default;
	TaggedMember(const T& v) : val(v) {}
	TaggedMember(const TaggedMember& v) = default;
	TaggedMember& operator=(const TaggedMember &rhs) = default;
	TaggedMember& operator=(const T& rhs) { val = rhs; return *this; }
	operator const T& () const { return val; }
	bool operator==(const TaggedMember &rhs) const { return val == rhs.val; }
	bool operator==(const T &rhs) const { return val == rhs; }
	bool operator<(const TaggedMember &rhs) const { return val < rhs.val; }

	static const auto tag = Tag.value;
	using kind = T;
};

struct UniversalType {
	template<typename T>
	operator T() {}
};

template<typename T>
consteval auto MemberCounter(auto ...Members) {
	if constexpr (requires { T{ Members... }; } == false)
		return sizeof...(Members) - 1;
	else
		return MemberCounter<T>(Members..., UniversalType{});
}

QDebug operator<<(QDebug debug, const NetworkResult &data)
{
	QDebugStateSaver saver(debug);

	if (data.isError())
	{
		debug.nospace() << "Error(code=" << data.error().code << ", desc=" << data.error().desc << ")";
	}
	else
	{
		debug.nospace() << "Success(document=" << data.value() << ")";
	}

	return debug;
}
