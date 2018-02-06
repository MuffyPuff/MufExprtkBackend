#include "mufexprtkbackend.h"

#include <cmath>

#include <QDebug>
#include <QStringLiteral>

#include "exprtk.hpp"

MufExprtkBackend::MufExprtkBackend(QObject* parent, const QString& in)
        : QThread(parent), _input(in), _hasnewinfo(false), _abort(false)
{
}

MufExprtkBackend::~MufExprtkBackend()
{
	_mutex.lock();
	_abort = true;

	foreach (symbol_t var, _variables) {
		delete var.second;
	}
	foreach (symbol_t con, _constants) {
		delete con.second;
	}

	_condnewinfoavail.wakeOne();
	_mutex.unlock();
	wait();
}

bool
MufExprtkBackend::addVariable(const QString& name, const double& value)
{
	QMutexLocker mutexlocker(&_mutex);
	_variables.append(symbol_t(name.toStdString(), new double(value)));
	_hasnewinfo = true;
	_condnewinfoavail.wakeOne();
	return true;
}

bool
MufExprtkBackend::addConstant(const QString& name, const double& value)
{
	QMutexLocker mutexlocker(&_mutex);
	_constants.append(symbol_t(name.toStdString(), new double(value)));
	_hasnewinfo = true;
	_condnewinfoavail.wakeOne();
	return true;
}

void
MufExprtkBackend::run()
{
	qDebug() << "calculating";

	symbol_table_t symbol_table;
	expression_t expression;
	parser_t parser;
	QList<symbol_t> variables;
	QList<symbol_t> constants;
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
		variables = _variables;
		constants = _constants;
		_hasnewinfo = false;
		_mutex.unlock();

		foreach (symbol_t var, variables) {
			symbol_table.add_variable(var.first, *var.second);
		}

		foreach (symbol_t con, constants) {
			symbol_table.add_constant(con.first, *con.second);
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

QList<MufExprtkBackend::symbol_t>
MufExprtkBackend::getVariables()
{
	return _variables;
}
