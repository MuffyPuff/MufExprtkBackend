#include "mufexprtkbackend.h"

#include <cmath>

#include <QDebug>
#include <QStringLiteral>

#include "../include/exprtk.hpp"

MufExprtkBackend::MufExprtkBackend(QObject* parent, const QString& in)
        : QThread(parent), _input(in), _hasnewinfo(false), _abort(false)
{
}

MufExprtkBackend::~MufExprtkBackend()
{
	_mutex.lock();
	_abort = true;

	//TODO: cast to symbol_t
//	destroy<num_t>(static_cast<QList<symbol_t<num_t>>*>(&_variables));
//	destroy<num_t>(static_cast<QList<symbol_t<num_t>>*>(&_constants));
//	destroy<vec_t>(static_cast<QList<symbol_t<vec_t>>*>(&_vectors));
//	destroy<str_t>(static_cast<QList<symbol_t<str_t>>*>(&_strings));
	destroy<num_t>(&_variables);
	destroy<num_t>(&_constants);
	destroy<vec_t>(&_vectors);
	destroy<str_t>(&_strings);

	_condnewinfoavail.wakeOne();
	_mutex.unlock();
	wait();
}

bool
MufExprtkBackend::addVariable(const QString& name, const num_t& value)
{
	QMutexLocker mutexlocker(&_mutex);
	_variables.append(
	        num_sym_t(
	                name.toStdString(),
	                value));
//	_symbol_table->add_variable(name.toStdString(), value);
	_hasnewinfo = true;
	_condnewinfoavail.wakeOne();
	return true;
}

bool
MufExprtkBackend::addConstant(const QString& name, const num_t& value)
{
	QMutexLocker mutexlocker(&_mutex);
	_constants.append(
	        num_sym_t(
	                name.toStdString(),
	                value));
	_hasnewinfo = true;
	_condnewinfoavail.wakeOne();
	return true;
}

bool
MufExprtkBackend::addString(const QString& name, const QString& value)
{
	QMutexLocker mutexlocker(&_mutex);
	_strings.append(
	        str_sym_t(
	                name.toStdString(),
	                value.toStdString()));
	_hasnewinfo = true;
	_condnewinfoavail.wakeOne();
	return true;
}

bool
MufExprtkBackend::addVector(const QString& name, const QVector<num_t>& values)
{
	QMutexLocker mutexlocker(&_mutex);
	_vectors.append(
	        vec_sym_t(
	                name.toStdString(),
	                values.toStdVector()));
	_hasnewinfo = true;
	_condnewinfoavail.wakeOne();
	return true;
}

bool
MufExprtkBackend::addFunction(const QString& name,
                              const QString& body,
                              const QStringList& vars)
{
	QMutexLocker mutexlocker(&_mutex);
	QList<str_t> std_vars;
	foreach (QString el, vars) {
		std_vars.append(el.toStdString());
	}
	_functions.append(
	        fun_sym_t(
	                name.toStdString(),
	                body.toStdString(),
	                std_vars));
	_hasnewinfo = true;
	_condnewinfoavail.wakeOne();
	return true;
}

// *INDENT-OFF*
template<typename T>
QList<MufExprtkBackend::symbol_t<T>>*
MufExprtkBackend::deepcopy(
        const QList<symbol_t<T>>& in,
        QList<symbol_t<T>>* out)
{
	destroy(out);
	foreach (symbol_t<T> el, in) {
		out->append(symbol_t<T>(el.name, *el.value));
	}
	return out;
}
// *INDENT-ON*

template<typename T>
void
MufExprtkBackend::destroy(QList<symbol_t<T>>* in)
{
	foreach (symbol_t<T> el, *in) {
		delete el.value;
		el.value = nullptr;
	}
	in->clear();
}

void
MufExprtkBackend::run()
{
	qDebug() << "calculating";
	typedef typename compositor_t::function         function_t;

	symbol_table_t symbol_table;
	expression_t expression;
	parser_t parser;
	compositor_t compositor(symbol_table);
	QList<fun_sym_t> functions;
	QList<num_sym_t> variables;
	QList<num_sym_t> constants;
	QList<str_sym_t> strings;
	QList<vec_sym_t> vectors;
	QString input;
//	parser.enable_unknown_symbol_resolver();

	for (;;) {
		_mutex.lock();
		bool abrt = _abort;
		_mutex.unlock();
		if (abrt) {
			return;
		}

		// get vars and input
		_mutex.lock();
		input = _input;
//		functions = _functions;
//		variables = _variables;
//		constants = _constants;
//		strings = _strings;
//		vectors = _vectors;

//		foreach (num_sym_t var, _variables) {
//			variables.append(num_sym_t(var.name, new num_t(*var.value)));
//		}

		//TODO: cast to symbol_t?
		deepcopy(_variables, &variables);
		deepcopy(_constants, &constants);
		deepcopy(_strings, &strings);
		deepcopy(_vectors, &vectors);
		functions = _functions;

		_hasnewinfo = false;
		_mutex.unlock();

		foreach (fun_sym_t fun, functions) {
			function_t fn = function_t()
			                .name(fun.name)
			                .expression(fun.body);
			foreach (str_t var, fun.vars) {
				fn.var(var);
			}
			compositor.add(fn);
		}
		foreach (num_sym_t var, variables) {
			symbol_table.add_variable(var.name, *var.value);
		}
		foreach (num_sym_t con, constants) {
			symbol_table.add_constant(con.name, *con.value);
		}
		foreach (str_sym_t str, strings) {
			symbol_table.add_stringvar(str.name, *str.value);
		}
		foreach (vec_sym_t vec, vectors) {
			symbol_table.add_vector(vec.name, *vec.value);
		}
		symbol_table.add_constants();
		expression.register_symbol_table(symbol_table);

		if (!parser.compile(input.toStdString(), expression)) {
			qDebug() << "expression compilation error...";
			for (int i = parser.error_count(); i > 0; --i) {
				error_list.prepend(parser.get_error(i - 1));
			}
			emit error();
			return;
		}

		_mutex.lock();
		bool abort = _abort;
		bool hasnewinfo = _hasnewinfo;
		_mutex.unlock();

		if (abort) {
			return;
		}
		if (hasnewinfo) {
			continue;
		}
		// TODO: update variables et al

		output = expression.value();
		emit resultAvailable(output);
//		return;

		_mutex.lock();
		_condnewinfoavail.wait(&_mutex);
		_mutex.unlock();
	}
}

bool
MufExprtkBackend::inputChanged(const QString& in)
{
	QMutexLocker mutexlocker(&_mutex);
	if (_input == in) {
		return false;
	}
	_input = in;
	_hasnewinfo = true;
	_condnewinfoavail.wakeOne();
	return true;
}

//using namespace MufExprtkBackend;

QList<MufExprtkBackend::fun_sym_t>
MufExprtkBackend::getFunctions()
{
	return _functions;
}

QList<MufExprtkBackend::num_sym_t>
MufExprtkBackend::getVariables()
{
	return _variables;
}

QList<MufExprtkBackend::str_sym_t>
MufExprtkBackend::getStrings()
{
	return _strings;
}

QList<MufExprtkBackend::vec_sym_t>
MufExprtkBackend::getVectors()
{
	return _vectors;
}
