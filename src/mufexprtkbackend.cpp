#include "mufexprtkbackend.h"

#include <cmath>

#include <QDebug>
#include <QStringLiteral>

#include "../include/exprtk.hpp"

MufExprtkBackend::MufExprtkBackend(QObject* parent, const QString& in)
        : QThread(parent), _input(in), _hasnewinfo(false), _abort(false)
{
	symbol_table_t unknown_var_symbol_table;
	unknown_var_symbol_table.add_constants();
	std::vector<std::string> var_list;
	unknown_var_symbol_table.get_variable_list(var_list);
	for (auto& el : var_list) {
//            qDebug() << QString::fromStdString(el)
//                     << unknown_var_symbol_table.variable_ref(el);
		_constants.append(
		        num_sym_t(
		                el,
		                unknown_var_symbol_table.variable_ref(el)));

	}
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
	destroy<num_t>(&_variables);
	destroy<num_t>(&_constants);
	destroy<vec_t>(&_vectors);
	destroy<str_t>(&_stringvars);

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
	for (QString el : vars) {
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
		for (symbol_t<T> el: in) {
			out->append(symbol_t<T>(el.name, *el.value));
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
		for (symbol_t<T> el : *in) {
//			qDebug() << "this should happen - d";
			delete el.value;
//			qDebug() << "this should happen - d";
			el.value = nullptr;
//			qDebug() << "this should happen - d";
		}
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
//	expression_t expression;
//	parser_t parser;
//	compositor_t compositor(symbol_table);


	for (;;) {
		qDebug("calculating");
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
		QString input = "";
		_mutex.lock();
		input = _input;
//		functions = _functions;
//		variables = _variables;
//		constants = _constants;
//		stringvars = _stringvars;
//		vectors = _vectors;

//		for (num_sym_t var, _variables) {
//			variables.append(num_sym_t(var.name, new num_t(*var.value)));
//		}

		QList<fun_sym_t> functions;
		QList<num_sym_t> variables;
		QList<num_sym_t> constants;
		QList<str_sym_t> stringvars;
		QList<vec_sym_t> vectors;
		deepcopy(_variables, &variables);
		deepcopy(_constants, &constants);
		deepcopy(_stringvars, &stringvars);
		deepcopy(_vectors, &vectors);
		functions = _functions;
//		qDebug() << "functions.size()" << functions.size();
//		qDebug() << "loc:" << QString::fromStdString(variables.at(
//		                        0).name) << *variables.at(0).value;
//		qDebug() << "glob:" << QString::fromStdString(_variables.at(
//		                        0).name) << *_variables.at(0).value;

		_hasnewinfo = false;
		_mutex.unlock();
		expression_t expression;
		parser_t parser;
		compositor_t compositor;
		parser.enable_unknown_symbol_resolver();

		std::vector<std::string> var_list;

		for (fun_sym_t fun : functions) {
			function_t fn = function_t()
			                .name(fun.name)
			                .expression(fun.body);
//			qDebug() << QString::fromStdString(fun.name) <<
//			         QString::fromStdString(fun.body);
			for (str_t var : fun.vars) {
				fn.var(var);
//				qDebug() << QString::fromStdString(var);
			}
//			qDebug() << "vars added:" << fn.v_.size();
//			qDebug() << "vars:";
//			for (std::string v, fn.v_) {
//				qDebug() << QString::fromStdString(v);
//			}
			compositor.add(fn);
		}
		symbol_table_t& symbol_table = compositor.symbol_table();
//		qDebug() << "functions in syt:" << symbol_table.function_count();
//		qDebug() << "functions in comp:" << compositor.symbol_table().function_count();
		for (num_sym_t var : variables) {
			symbol_table.add_variable(var.name, *var.value);
		}
		for (num_sym_t con : constants) {
			symbol_table.add_constant(con.name, *con.value);
		}
		for (str_sym_t str : stringvars) {
			symbol_table.add_stringvar(str.name, *str.value);
		}
		for (vec_sym_t vec : vectors) {
			symbol_table.add_vector(vec.name, *vec.value);
		}
		symbol_table_t unknown_var_symbol_table;
//		symbol_table.add_constants(); // in constructor
		expression.register_symbol_table(unknown_var_symbol_table);
		expression.register_symbol_table(symbol_table);

		if (!parser.compile(input.toStdString(), expression)) {
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

		unknown_var_symbol_table.get_variable_list(var_list);
		for (auto& el : var_list) {
			qDebug() << QString::fromStdString(el) \
			         << unknown_var_symbol_table.variable_ref(el);
			if (!unknown_var_symbol_table.is_constant_node(el)) {
				qDebug("append");
				_variables.append(
				        num_sym_t(
				                el,
//				                unknown_var_symbol_table.variable_ref(el)));
				                unknown_var_symbol_table.get_variable(el)->value()));

//				qDebug("add");
//				symbol_table.add_variable(el, *variables.back().value);
//				qDebug("remove");
//				unknown_var_symbol_table.remove_variable(el);

				_mutex.lock();
				_hasnewinfo = true;
				_mutex.unlock();
			}
		}

		qDebug("just in case i don't get here");
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

		qDebug("value");
		r = expression.value();
//		qDebug() << "loc:" << QString::fromStdString(variables.at(
//		                        0).name) << *variables.at(0).value;
//		qDebug() << "glob:" << QString::fromStdString(_variables.at(
//		                        0).name) << *_variables.at(0).value;

		_mutex.lock();
		// TODO: update variables et al
//		compositor.clear();
		qDebug("deep copy");
		deepcopy(variables, &_variables);
		deepcopy(constants, &_constants);
		deepcopy(stringvars, &_stringvars);
		deepcopy(vectors, &_vectors);
//		_hasnewinfo = true;
//		qDebug() << "loc:" << QString::fromStdString(variables.at(
//		                        0).name) << *variables.at(0).value;
//		qDebug() << "glob:" << QString::fromStdString(_variables.at(
//		                        0).name) << *_variables.at(0).value;
		output = r;
		_mutex.unlock();

		qDebug("emit");
		emit resultAvailable(output);
cleanup:
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
//		var_list.clear();
		qDebug("destroy");
		destroy<num_t>(&variables);
		destroy<num_t>(&constants);
		destroy<str_t>(&stringvars);
		destroy<vec_t>(&vectors);

//		qDebug() << "this happens";
//		qDebug() << "this should happen";
//		return; // wtf am i doing

		_mutex.lock();
//		qDebug() << "this should happen";
		_condnewinfoavail.wait(&_mutex);
//		qDebug() << "this might happen";
		_mutex.unlock();
		expression.release();
		qDebug("this might happen");
	}
	qDebug("literaly impossible");
	qFatal("wut");
//	Q_UNREACHABLE();
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
