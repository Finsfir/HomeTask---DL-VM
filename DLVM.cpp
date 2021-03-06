#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <utility>

template <typename K> class env;

class expression {
public:
	env<std::string> *thisenv;
	virtual int getValue() { return 0; };
	virtual expression *eval() { return NULL; };
	virtual ~expression() { };
	virtual std::string get_id() { throw "ERROR"; };
};

template <typename K> class env {
	typedef std::map<K, expression*> Map;
	Map mas;
public:
	env() {

	}
	env& operator << (const std::pair<K, expression*> &v)
	{
		auto res = mas.find(v.first);
		if (res != mas.end()) {
			mas.erase(v.first);
		}
		mas.insert(v);
		return *this;
	}
	void erase(std::pair<K, expression*> &v) {
		//delete v.second;
		mas.erase(v.first);
	}
	expression *fromEnv(K id) {
		auto res = mas.find(id);
		if (res != mas.end()) {
			return res->second;
		}
		return NULL;
	}
};

class val : public expression {
	int i;
public:
	val(int in) {
		i = in;
	}
	int getValue() {
		return i;
	}
	expression *eval() {
		return this;
	}
};

template <typename K>class var : public expression {
	K k;
public:
	var(K in) {
		k = in;
	}
	int getValue() {
		expression *res = NULL;
		if (thisenv != NULL)
			res = thisenv->fromEnv(k);
		if (res == NULL) {
			throw "ERROR";
		}
		res->thisenv = thisenv;
		return res->getValue();
	}
	expression *eval() {
		expression *res = NULL;
		if (thisenv != NULL)
			res = thisenv->fromEnv(k);
		if (res == NULL)
			throw "ERROR";
		res->thisenv = thisenv;
		return res->eval();
	}
	std::string get_id() {
		return k;
	};
};

class add : public expression {
	expression *first;
	expression *second;
public:
	add(expression *f, expression *s) {
		first = f;
		second = s;
	}
	int getValue() {
		throw "ERROR";
	}
	expression *eval() {
		first->thisenv = thisenv;
		second->thisenv = thisenv;
		expression *e1 = first->eval();
		expression *e2 = second->eval();
		e1->thisenv = thisenv;
		e2->thisenv = thisenv;
		expression *res = new val(e1->getValue() + e2->getValue());
		return res;
	}
};

class ex_if : public expression {
	expression* first;
	expression* second;
	expression* _then;
	expression* _else;
public:
	ex_if(expression *f, expression *s, expression *e1, expression *e2) {
		first = f;
		second = s;
		_then = e1;
		_else = e2;
	}
	int getValue() {
		throw "ERROR";
	}
	expression *eval() {
		first->thisenv = thisenv;
		second->thisenv = thisenv;
		expression *e1 = first->eval();
		expression *e2 = second->eval();
		e1->thisenv = thisenv;
		e2->thisenv = thisenv;
		if (e1->getValue() > e2->getValue()) {
			_then->thisenv = thisenv;
			return _then->eval();
		}
		else
		{
			_else->thisenv = thisenv;
			return _else->eval();
		}
	}
};

template <typename K>class let : public expression {
	K first;
	expression *second;
	expression *func;
public:
	let(K id, expression *f, expression *s) {
		first = id;
		second = f;
		func = s;

	}
	int getValue() {
		throw "ERROR";
	}
	expression *eval() {
		env<K> *thsenv = new env<K>(*thisenv);
		std::pair<K, expression*> p = std::pair<K, expression*>(first, second);
		*thsenv << p;
		second->thisenv = thsenv;
		func->thisenv = thsenv;
		expression *res = func->eval();
		delete thsenv;
		return res;
	}
};

template <typename K>class function : public expression {
	K k;
	expression *second;
public:
	K getK() {
		return k;
	}
	function(K id, expression *second) {
		thisenv = new env<std::string>();
		this->second = second;
		second->thisenv = thisenv;
		k = id;
	}
	int getValue() {
		throw "ERROR";
	}
	expression *eval() {
		expression *ex = thisenv->fromEnv(k);
		expression *res = new let<K>(k, ex, second);
		res->thisenv = thisenv;
		return res->eval();
	}
	std::string get_id() { return k; };

};

template <typename K>class call : public expression {
	expression *first;
	expression *second;
public:
	call(expression *first, expression *second) {
		this->first = first;
		this->second = second;

	}
	int getValue() {
		throw "ERROR";
	}
	expression *eval() {
		K r = first->get_id();
		expression *f = thisenv->fromEnv(r);
		std::pair<K, expression*> p = std::pair<K, expression*>(f->get_id(), second);
		*thisenv << p;
		first->thisenv = thisenv;
		expression *res = first->eval();
		res->eval();
		thisenv->erase(p);
		return res;
	}
};

template <typename K>expression *strPrs(std::ifstream& input) {
	std::string str;
	input >> str;
	if ((str)[0] == '(')
		str.erase(0, 1);
	int n = str.find(' ');
	std::string s = str.substr(0, n);
	expression *Ex = NULL;
	if (s == "val") {
		int value;
		input >> str;
		int num = str.find(')');
		std::stringstream(str.substr(0, num)) >> value;
		Ex = new val(value);
	}
	else if (s == "var") {
		std::string value;
		input >> str;
		int num = str.find(')');
		std::stringstream(str.substr(0, num)) >> value;
		Ex = new var<std::string>(value);
	}
	else if (s == "add") {
		std::string value;
		expression *first = strPrs<K>(input);
		expression *second = strPrs<K>(input);
		Ex = new add(first, second);
	}
	else if (s == "if") {
		expression *f = strPrs<K>(input);
		expression *s = strPrs<K>(input);
		input >> str;
		expression *th = strPrs<K>(input);
		input >> str;
		expression *el = strPrs<K>(input);
		input >> str;
		Ex = new ex_if(f, s, th, el);
	}
	else if (s == "let") {
		K first;
		input >> first;
		input >> str;
		expression *e = strPrs<K>(input);
		input >> str;
		expression *second = strPrs<K>(input);
		Ex = new let<K>(first, e, second);
	}
	else if (s == "function") {
		K k;
		input >> k;
		expression *second = strPrs<K>(input);
		Ex = new function<K>(k, second);
	}
	else if (s == "call") {
		expression *first = strPrs<K>(input);
		expression *second = strPrs<K>(input);
		Ex = new call<K>(first, second);
	}
	return Ex;
}

int main()
{
	std::ifstream input;
	input.open("input.txt", std::ifstream::in);
	std::fstream output;
	output.open("output.txt", std::fstream::out);
	expression *ex;
	ex = strPrs<std::string>(input);
	ex->thisenv = new env<std::string>();
	try {
		expression *res = ex->eval();
		int i = res->getValue();
		output << "(val ";
		output << i;
		output << ")";
	}
	catch (...) {
		output << "ERROR";
	}
	input.close();
	output.close();
}