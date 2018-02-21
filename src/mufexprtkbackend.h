#ifndef MUFEXPRTKBACKEND_H
#define MUFEXPRTKBACKEND_H

#include <QObject>
#include <QThread>
#include <QWaitCondition>
#include <QString>
#include <QList>
#include <QPair>
#include <QMutex>
#include <QMutexLocker>

namespace exprtk
{
template <typename T>
class symbol_table;
template <typename T>
class expression;
template <typename T>
class parser;
template <typename T>
class function_compositor;
/*{
public:
	struct function;
};*/

namespace parser_error
{
struct type;
}
}

//using std::string;

class MufExprtkBackend : public QThread
{
	Q_OBJECT

protected:

// generic
	typedef double                                  num_t;
	typedef std::string                             str_t;
	typedef std::vector<num_t>                      vec_t;
// exprtk calculation
	typedef exprtk::symbol_table<num_t>             symbol_table_t;
	typedef exprtk::expression<num_t>               expression_t;
	typedef exprtk::parser<num_t>                   parser_t;
	typedef exprtk::function_compositor<num_t>      compositor_t;
//	typedef typename compositor_t::function         function_t;
// symbols
//	typedef QPair<str_t, num_t*>                    num_sym_t;
//	typedef QPair<str_t, str_t*>                    str_sym_t;
//	typedef QPair<str_t, std::vector<num_t>*>       vec_sym_t;
//	typedef QList<str_t, str_t, QList<str_t>>       fun_sym_t;
// exprtk error
	typedef exprtk::parser_error::type              parse_err_t;
public: // temp
	template<typename T>
	struct symbol_t {
		symbol_t(str_t n, const T& v)
		        : name(n), value(new T(v)) {}
		str_t           name;
		T*              value;
	};
protected: // temp
	typedef symbol_t<num_t> num_sym_t;
	typedef symbol_t<str_t> str_sym_t;
	typedef symbol_t<vec_t> vec_sym_t;

//	struct num_sym_t : public symbol_t<num_t> {
////            num_sym_t(str_t n, const num_t& v) : symbol_t(n, v) {}
//	};
//	struct str_sym_t : public symbol_t<str_t> {
////            str_sym_t(str_t n, const str_t& v) : symbol_t(n, v) {}
//	};
//	struct vec_sym_t : public symbol_t<vec_t> {
////            vec_sym_t(str_t n, const vec_t& v) : symbol_t(n, v) {}
//	};

	struct fun_sym_t {
		fun_sym_t(str_t n, str_t b, QList<str_t> v)
		        : name(n), body(b), vars(v) {}
		str_t           name;
		str_t           body;
		QList<str_t>    vars;
	};

	// *INDENT-OFF*
	template<typename T>
	QList<MufExprtkBackend::symbol_t<T>>*
	deepcopy(const QList<symbol_t<T>>& in, QList<symbol_t<T>>* out);
	// *INDENT-ON*
	template<typename T>
	void destroy(QList<symbol_t<T>>* in);

public:
	explicit MufExprtkBackend(QObject* parent, const QString& in);
	~MufExprtkBackend();

	double          output;
	QList<parse_err_t> error_list;

	bool            addVariable(const QString& name, const num_t& value);
	bool            addConstant(const QString& name, const num_t& value);
	bool            addString(const QString& name, const QString& value);
	bool            addVector(const QString& name, const QVector<num_t>& values);
	bool            addFunction(const QString& name, const QString& body,
	                            const QStringList& vars);

	QList<fun_sym_t> getFunctions();
	QList<num_sym_t> getVariables();
	QList<str_sym_t> getStrings();
	QList<vec_sym_t> getVectors();

	void            run();

signals:
	void            resultAvailable(num_t);
	void            error();

public slots:
	bool            inputChanged(const QString& in);

protected:
	QList<fun_sym_t> _functions;
	QList<num_sym_t> _variables;
	QList<num_sym_t> _constants;
	QList<str_sym_t> _strings;
	QList<vec_sym_t> _vectors;

	QString         _input;

	QMutex          _mutex;
	QWaitCondition  _condnewinfoavail;

	bool            _hasnewinfo;
	bool            _abort;

};

#endif // MUFEXPRTKBACKEND_H
