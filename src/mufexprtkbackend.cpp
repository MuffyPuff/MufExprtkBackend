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
//	destroy<str_t>(static_cast<QList<symbol_t<str_t>>*>(&_stringvars));
//	destroy<num_t>(&_variables);
//	destroy<num_t>(&_constants);
//	destroy<vec_t>(&_vectors);
//	destroy<str_t>(&_stringvars);

	_condnewinfoavail.wakeOne();
	_mutex.unlock();
	wait();
}

bool
MufExprtkBackend::addVariable(const QString& name, const num_t& value)
{
	QMutexLocker mutexlocker(&_mutex);
	qDebug() << name << value;
	_variables.append(
	        num_sym_t(
	                name.toStdString(),
	                value));
//	_symbol_table->add_variable(name.toStdString(), value);
//	qDebug() <<  QString::fromStdString(_variables.at(0).name) << *_variables.at(
//	                 0).value;
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
MufExprtkBackend::addStringvar(const QString& name, const QString& value)
{
	QMutexLocker mutexlocker(&_mutex);
	_stringvars.append(
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
//	qDebug() << _functions.size();
//	qDebug() << "vars:" << vars.size();
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
//	qDebug() << "this should happen";
	destroy(out);
	if (in.size() > 0) {
//		qDebug() << "this should happen";
		foreach (symbol_t<T> el, in) {
			out->append(symbol_t<T>(el.name, T(el.value)));
		}
	}
	return out;
}
// *INDENT-ON*

template<typename T>
void
MufExprtkBackend::destroy(QList<symbol_t<T>>* in)
{
	if (in->size() > 0) {
//		qDebug() << "this should happen - d";
//		foreach (symbol_t<T> el, *in) {
////                    qDebug() << "this should happen - d";
//			delete el.value;
////                    qDebug() << "this should happen - d";
//			el.value = nullptr;
////                    qDebug() << "this should happen - d";
//		}
		in->clear();
//		qDebug() << "this should happen - d";
	}
}

void
MufExprtkBackend::run()
{
//	qDebug() << "calculating";
	typedef typename compositor_t::function         function_t;

//	symbol_table_t& symbol_table;
	expression_t expression;
	parser_t parser;
//	compositor_t compositor(symbol_table);
	QList<fun_sym_t> functions;
	QList<num_sym_t> variables;
	QList<num_sym_t> constants;
	QList<str_sym_t> stringvars;
	QList<vec_sym_t> vectors;
	QString input;
	parser.enable_unknown_symbol_resolver();

	for (;;) {
		qDebug() << "calculating";
		bool abort = false;
		bool hasnewinfo = false;
		double r = false;
		_mutex.lock();
		abort = _abort;
		_mutex.unlock();
		if (abort) {
			return;
		}

		// get vars and input
		_mutex.lock();
		input = _input;
//		functions = _functions;
//		variables = _variables;
//		constants = _constants;
//		stringvars = _stringvars;
//		vectors = _vectors;

//		foreach (num_sym_t var, _variables) {
//			variables.append(num_sym_t(var.name, new num_t(*var.value)));
//		}

		//TODO: cast to symbol_t?
		deepcopy(_variables, &variables);
		deepcopy(_constants, &constants);
		deepcopy(_stringvars, &stringvars);
		deepcopy(_vectors, &vectors);
		functions = _functions;
//		qDebug() << "functions.size()" << functions.size();
//		qDebug() << "loc:" << QString::fromStdString(variables.at(
//		                        0).name) << variables.at(0).value;
//		qDebug() << "glob:" << QString::fromStdString(_variables.at(
//		                        0).name) << _variables.at(0).value;

		_hasnewinfo = false;
		_mutex.unlock();
		compositor_t compositor;

		foreach (fun_sym_t fun, functions) {
			function_t fn = function_t()
			                .name(fun.name)
			                .expression(fun.body);
//			qDebug() << QString::fromStdString(fun.name) <<
//			         QString::fromStdString(fun.body);
			foreach (str_t var, fun.vars) {
				fn.var(var);
//				qDebug() << QString::fromStdString(var);
			}
//			qDebug() << "vars added:" << fn.v_.size();
//			qDebug() << "vars:";
//			foreach (std::string v, fn.v_) {
//				qDebug() << QString::fromStdString(v);
//			}
			compositor.add(fn);
		}
		symbol_table_t& symbol_table = compositor.symbol_table();
//		qDebug() << "functions in syt:" << symbol_table.function_count();
//		qDebug() << "functions in comp:" << compositor.symbol_table().function_count();
		foreach (num_sym_t var, variables) {
			symbol_table.add_variable(var.name, var.value);
		}
		foreach (num_sym_t con, constants) {
			symbol_table.add_constant(con.name, con.value);
		}
		foreach (str_sym_t str, stringvars) {
			symbol_table.add_stringvar(str.name, str.value);
		}
		foreach (vec_sym_t vec, vectors) {
			symbol_table.add_vector(vec.name, vec.value);
		}
		qDebug() << "vars in syt:" << symbol_table.variable_count();
		qDebug() << "vars in comp:" << compositor.symbol_table().variable_count();
		symbol_table.add_constants();
		expression.register_symbol_table(symbol_table);
		qDebug() << "set up syt done";
		bool b = false;
		b = parser.compile(input.toStdString(), expression);
		qDebug() << "parsed";

		if (!b) {
			qDebug() << "expression compilation error...";
//			qDebug() << QString::fromStdString(parser.error());
			for (int i = parser.error_count(); i > 0; --i) {
				error_list.prepend(parser.get_error(i - 1));
				qDebug() << QString::fromStdString(parser.get_error(i - 1).diagnostic);
			}
			emit error();
//			_mutex.lock();
			goto cleanup;
		}

		_mutex.lock();
		abort = _abort;
		hasnewinfo = _hasnewinfo;
		_mutex.unlock();

		if (abort) {
			return;
		}
		if (hasnewinfo) {
			continue;
		}
		qDebug() << "before .value()";
		r = expression.value();
//		qDebug() << "loc:" << QString::fromStdString(variables.at(
//		                        0).name) << variables.at(0).value;
//		qDebug() << "glob:" << QString::fromStdString(_variables.at(
//		                        0).name) << _variables.at(0).value;

		_mutex.lock();
		// TODO: update variables et al
//		compositor.clear();
		deepcopy(variables, &_variables);
		deepcopy(constants, &_constants);
		deepcopy(stringvars, &_stringvars);
		deepcopy(vectors, &_vectors);
//		_variables.clear();
//		_constants.clear();
//		_stringvars.clear();
//		_vectors.clear();
		// *INDENT-OFF*
//		{
////		qDebug("yo");
//		std::list<std::string> l{};
//		symbol_table.get_variable_list(l);
//		for (std::string el : l) {
////			qDebug() << QString::fromStdString(el) << symbol_table.get_variable(el)->value();
//			if (!symbol_table.is_constant_node(el)) {
//				_variables.append(
//				        symbol_t<num_t>(
//				                el,
//				                symbol_table.get_variable(el)->value()));
//				qDebug() << QString::fromStdString(el) << symbol_table.get_variable(el)->value();
//			} else {
////				_constants.append(
////				        symbol_t<num_t>(
////				                el,
////				                symbol_table.get_variable(el)->value()));
//			}
//		}
//		l.clear();
//		symbol_table.get_stringvar_list(l);
//		for (auto el : l) {
////			if (symbol_table.is_constant_string(el)) {
////				continue;
////			}
//			_stringvars.append(
//			        symbol_t<str_t>(
//			                el,
//			                symbol_table.get_stringvar(el)->str()));
//		}
//		// i give up
//		// there is no function that returns whole vectors...
//		// thanks exprtk /s
//		deepcopy(vectors, &_vectors);
//		qDebug("yottsu");
//		}
		// *INDENT-ON*
//		deepcopy(variables, &_variables);
//		deepcopy(constants, &_constants);
//		deepcopy(stringvars, &_stringvars);
//		deepcopy(vectors, &_vectors);
//		_hasnewinfo = true;
//		qDebug() << "loc:" << QString::fromStdString(variables.at(
//		                        0).name) << *variables.at(0).value;
//		qDebug() << "glob:" << QString::fromStdString(_variables.at(
//		                        0).name) << *_variables.at(0).value;
		output = r;
		_mutex.unlock();

		emit resultAvailable(output);
cleanup:
		compositor.clear();
		_mutex.lock();
		abort = _abort;
		hasnewinfo = _hasnewinfo;
		_mutex.unlock();

		if (abort) {
			return;
		}
		if (hasnewinfo) {
			continue;
		}
//		qDebug() << "this happens";
//		destroy<num_t>(&variables);
//		destroy<num_t>(&constants);
//		destroy<vec_t>(&vectors);
//		destroy<str_t>(&stringvars);

//		qDebug() << "this happens";
//		qDebug() << "this should happen";
//		return; // wtf am i doing

		_mutex.lock();
//		qDebug() << "this should happen";
		_condnewinfoavail.wait(&_mutex);
//		qDebug() << "this might happen";
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
//	qDebug() << QString::fromStdString(_variables.at(0).name) << *_variables.at(
//	                 0).value;
	return _variables;
}

QList<MufExprtkBackend::num_sym_t>
MufExprtkBackend::getConstants()
{
	return _constants;
}

QList<MufExprtkBackend::str_sym_t>
MufExprtkBackend::getStringvars()
{
	return _stringvars;
}

QList<MufExprtkBackend::vec_sym_t>
MufExprtkBackend::getVectors()
{
	return _vectors;
}
