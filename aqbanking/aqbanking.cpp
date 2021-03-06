#pragma GCC diagnostic ignored "-Wwrite-strings"
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#ifdef DEBUG
#define Py_DEBUG 1
#endif

#include <Python.h>
#include <datetime.h>
#include "unicodeobject.h"
#include "structmember.h"
#include <aqbanking/banking.h>
#include <aqbanking/jobgetbalance.h>
#include <aqbanking/jobgettransactions.h>
#include "pyaqhandler.hpp"

/**
 * Here are some exceptions.
 */
static PyObject *AccountNotFound;
static PyObject *ExecutionFailed;

/**
 * Module specific AQ Handler.
 */
static PyAqHandler* aqh = NULL;
static AB_BANKING *ab = NULL;

/**
 * This is the structure for the Account class.
 */
typedef struct {
	PyObject_HEAD
	PyObject *no;
	PyObject *name;
	PyObject *description;
	PyObject *bank_code;
	PyObject *bank_name;

	PyAqHandler *aqh;
	AB_BANKING *ab;

} aqbanking_Account;

/**
 * This is the structure for a transaction.
 */
typedef struct {
	PyObject_HEAD
	PyObject *date;
	PyObject *localAccount;
	PyObject *localBank;
	PyObject *localIban;
	PyObject *localBic;
	PyObject *remoteAccount;
	PyObject *remoteBank;
	PyObject *remoteIban;
	PyObject *remoteBic;
	PyObject *purpose;
	PyObject *value;
	PyObject *currency;
	int state;

} aqbanking_Transaction;

int AB_create(aqbanking_Account *acct = NULL) {

		int rv = 0;

		//Initialisierungen GWEN
		if (acct == NULL) {
			GWEN_Gui_SetGui(aqh->getCInterface());
		} else {
			GWEN_Gui_SetGui(acct->aqh->getCInterface());
		}

		// Initialisierungen AB
		if (acct == NULL) {
			ab = AB_Banking_new("python-aqbanking", 0, AB_BANKING_EXTENSION_NONE);
			rv = AB_Banking_Init(ab);
		} else {
			acct->ab = AB_Banking_new("python-aqbanking", 0, AB_BANKING_EXTENSION_NONE);
			rv = AB_Banking_Init(acct->ab);
		}
		if (rv) {
				PyErr_SetObject(AqBankingInitializeError, PyUnicode_FromFormat("Could not initialize (%d).", rv));
				return 2;
		}
		if (acct == NULL) {
			rv = AB_Banking_OnlineInit(ab);
		} else {
			rv = AB_Banking_OnlineInit(acct->ab);
		}
		if (rv) {
				PyErr_SetObject(AqBankingInitializeError, PyUnicode_FromFormat("Could not do online initialize (%d).", rv));
				return 2;
		}
		
		//Setzen der Call-backs
		/*GWEN_Gui_SetProgressLogFn(this->gui, zProgressLog);
		GWEN_Gui_SetCheckCertFn(this->gui, zCheckCert);
		GWEN_Gui_SetMessageBoxFn(this->gui, zMessageBox);
		GWEN_Gui_SetGetPasswordFn(this->gui, zPasswordFn);*/

		return 0;
}

int AB_free(aqbanking_Account *acct = NULL) {
		int rv = 0;

		if (acct == NULL) {
			rv = AB_Banking_OnlineFini(ab);
		} else {
			rv = AB_Banking_OnlineFini(acct->ab);
		}
		if (rv) 
		{
				PyErr_SetObject(AqBankingInitializeError, PyUnicode_FromFormat("Could not do online deinit. (%d).", rv));
				return 3;
		}

		if (acct == NULL) {
			rv = AB_Banking_Fini(ab);
		} else {
			rv = AB_Banking_Fini(acct->ab);
		}
		if (rv) 
		{
				PyErr_SetObject(AqBankingInitializeError, PyUnicode_FromFormat("Could not do deinit. (%d).", rv));
				return 3;
		}

		if (acct == NULL) {
			AB_Banking_free(ab);
		} else {
			AB_Banking_free(acct->ab);
		}
		return 0;
}

//***** HERE THE AQBANKING PYTHON ITSELF STARTS ****
static void aqbanking_Transaction_dealloc(aqbanking_Transaction* self)
{
	Py_XDECREF(self->date);
	Py_XDECREF(self->localAccount);
	Py_XDECREF(self->localBank);
	Py_XDECREF(self->localIban);
	Py_XDECREF(self->localBic);
	Py_XDECREF(self->remoteAccount);
	Py_XDECREF(self->remoteBank);
	Py_XDECREF(self->remoteIban);
	Py_XDECREF(self->remoteBic);
	Py_XDECREF(self->purpose);
	Py_XDECREF(self->value);
	Py_XDECREF(self->currency);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *aqbanking_Transaction_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	aqbanking_Transaction *self;

	self = (aqbanking_Transaction *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->date = PyUnicode_FromString("");
		if (self->date == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->localAccount = PyUnicode_FromString("");
		if (self->localAccount == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->localBank = PyUnicode_FromString("");
		if (self->localBank == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->localIban = PyUnicode_FromString("");
		if (self->localIban == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->localBic = PyUnicode_FromString("");
		if (self->localBic == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->remoteAccount = PyUnicode_FromString("");
		if (self->remoteAccount == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->remoteBank = PyUnicode_FromString("");
		if (self->remoteBank == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->remoteIban = PyUnicode_FromString("");
		if (self->remoteIban == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->remoteBic = PyUnicode_FromString("");
		if (self->remoteBic == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->purpose = PyUnicode_FromString("");
		if (self->purpose == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		double startValue = 0.0;
		self->value = PyFloat_FromDouble(startValue);
		if (self->value == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->currency = PyUnicode_FromString("EUR");
		if (self->currency == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->state = 0;
	}

	return (PyObject *)self;
}

static int aqbanking_Transaction_init(aqbanking_Transaction *self, PyObject *args, PyObject *kwds)
{
	PyObject *date=NULL, *localIban=NULL, *localBic=NULL, *remoteIban=NULL, *remoteBic=NULL;
	PyObject *purpose=NULL, *value=NULL, *currency=NULL, *tmp;
	static char *kwlist[] = {"date", "localIban", "localBic", "remoteIban", "remoteBic", "purpose", "value", "currency", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOOOOOOO", kwlist,
									  &date, &localIban, &localBic, &remoteIban, &remoteBic, purpose, value, currency))
	{
		return -1;
	}

	if (date) {
		tmp = self->date;
		Py_INCREF(date);
		self->date = date;
		Py_XDECREF(tmp);
	}

	if (localIban) {
		tmp = self->localIban;
		Py_INCREF(localIban);
		self->localIban = localIban;
		Py_XDECREF(tmp);
	}

	if (localBic) {
		tmp = self->localBic;
		Py_INCREF(localBic);
		self->localBic = localBic;
		Py_XDECREF(tmp);
	}

	if (remoteIban) {
		tmp = self->remoteIban;
		Py_INCREF(remoteIban);
		self->remoteIban = remoteIban;
		Py_XDECREF(tmp);
	}

	if (remoteBic) {
		tmp = self->remoteBic;
		Py_INCREF(remoteBic);
		self->remoteBic = remoteBic;
		Py_XDECREF(tmp);
	}

	if (purpose) {
		tmp = self->purpose;
		Py_INCREF(purpose);
		self->purpose = purpose;
		Py_XDECREF(tmp);
	}

	if (value) {
		tmp = self->value;
		Py_INCREF(value);
		self->value = value;
		Py_XDECREF(tmp);
	}

	if (currency) {
		tmp = self->currency;
		Py_INCREF(currency);
		self->currency = currency;
		Py_XDECREF(tmp);
	}

	return 0;
}

static PyMemberDef aqbanking_Transaction_members[] = {
	{"date", T_OBJECT_EX, offsetof(aqbanking_Transaction, date), 0, "Date of transaction"},
	{"localAccount", T_OBJECT_EX, offsetof(aqbanking_Transaction, localAccount), 0, "Local account no."},
	{"localBank", T_OBJECT_EX, offsetof(aqbanking_Transaction, localBank), 0, "Local bank code"},
	{"localIban", T_OBJECT_EX, offsetof(aqbanking_Transaction, localIban), 0, "Local IBAN"},
	{"localBic", T_OBJECT_EX, offsetof(aqbanking_Transaction, localBic), 0, "Local BIC"},
	{"remoteAccount", T_OBJECT_EX, offsetof(aqbanking_Transaction, remoteAccount), 0, "Remote account no."},
	{"remoteBank", T_OBJECT_EX, offsetof(aqbanking_Transaction, remoteBank), 0, "Remote8 bank code"},
	{"remoteIban", T_OBJECT_EX, offsetof(aqbanking_Transaction, remoteIban), 0, "Remote IBAN"},
	{"remoteBic", T_OBJECT_EX, offsetof(aqbanking_Transaction, remoteBic), 0, "Remote BIC"},
	{"purpose", T_OBJECT_EX, offsetof(aqbanking_Transaction, purpose), 0, "Purpose of transaction"},
	{"value", T_OBJECT_EX, offsetof(aqbanking_Transaction, value), 0, "Value"},
	{"currency", T_OBJECT_EX, offsetof(aqbanking_Transaction, currency), 0, "Currency (by default EUR)"},
	{"state", T_INT, offsetof(aqbanking_Transaction, state), 0, "State of the transaction"},
	{NULL}
};

static PyMethodDef aqbanking_Transaction_methods[] = {
	/*{"name", (PyCFunction)aqbanking_Transaction_name, METH_NOARGS,
	},*/
	{NULL}  /* Sentinel */
};

static PyTypeObject aqbanking_TransactionType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"aqbanking.Transaction", /* tp_name */
	sizeof(aqbanking_Transaction), /* tp_basicsize */
	0, /* tp_itemsize */
	(destructor)aqbanking_Transaction_dealloc, /* tp_dealloc */
	0, /* tp_print */
	0, /* tp_getattr */
	0, /* tp_setattr */
	0, /* tp_reserved */
	0, /* tp_repr */
	0, /* tp_as_number */
	0, /* tp_as_sequence */
	0, /* tp_as_mapping */
	0, /* tp_hash  */
	0, /* tp_call */
	0, /* tp_str */
	0, /* tp_getattro */
	0, /* tp_setattro */
	0, /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"Transaction", /* tp_doc */
	0, /* tp_traverse */
	0, /* tp_clear */
	0, /* tp_richcompare */
	0, /* tp_weaklistoffset */
	0, /* tp_iter */
	0, /* tp_iternext */
	aqbanking_Transaction_methods, /* tp_methods */
	aqbanking_Transaction_members, /* tp_members */
	0, /* tp_getset */
	0, /* tp_base */
	0, /* tp_dict */
	0, /* tp_descr_get */
	0, /* tp_descr_set */
	0, /* tp_dictoffset */
	(initproc)aqbanking_Transaction_init, /* tp_init */
	0, /* tp_alloc */
	aqbanking_Transaction_New, /* tp_new */
};

/*
 * HERE THE ACCOUNT STARTS!
 */
static void aqbanking_Account_dealloc(aqbanking_Account* self)
{
	Py_XDECREF(self->no);
	Py_XDECREF(self->name);
	Py_XDECREF(self->description);
	Py_XDECREF(self->bank_code);
	Py_XDECREF(self->bank_name);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *aqbanking_Account_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	aqbanking_Account *self;

	self = (aqbanking_Account *)type->tp_alloc(type, 0);
	if (self != NULL) {
		self->no = PyUnicode_FromString("");
		if (self->no == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->name = PyUnicode_FromString("");
		if (self->name == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->description = PyUnicode_FromString("");
		if (self->description == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->bank_code = PyUnicode_FromString("");
		if (self->bank_code == NULL) {
			Py_DECREF(self);
			return NULL;
		}

		self->bank_name = PyUnicode_FromString("");
		if (self->bank_name == NULL) {
			Py_DECREF(self);
			return NULL;
		}

	}

	self->aqh = NULL;

	return (PyObject *)self;
}

static int aqbanking_Account_init(aqbanking_Account *self, PyObject *args, PyObject *kwds)
{
	PyObject *no=NULL, *name=NULL, *description=NULL, *bank_code=NULL, *bank_name=NULL, *tmp;
	static char *kwlist[] = {"no", "name", "description", "bank_code", "bank_name", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOOOO", kwlist,
									  &no, &name, &description, &bank_code, &bank_name))
		return -1;

	if (no) {
		tmp = self->no;
		Py_INCREF(no);
		self->no = no;
		Py_XDECREF(tmp);
	}

	if (name) {
		tmp = self->name;
		Py_INCREF(name);
		self->name = name;
		Py_XDECREF(tmp);
	}

	if (description) {
		tmp = self->description;
		Py_INCREF(description);
		self->description = description;
		Py_XDECREF(tmp);
	}

	if (bank_code) {
		tmp = self->bank_code;
		Py_INCREF(bank_code);
		self->bank_code = bank_code;
		Py_XDECREF(tmp);
	}

	if (bank_name) {
		tmp = self->bank_name;
		Py_INCREF(bank_name);
		self->bank_name = bank_name;
		Py_XDECREF(tmp);
	}

	self->aqh = new PyAqHandler();

	return 0;
}

static PyObject *aqbanking_Account_name(aqbanking_Account* self)
{
	if (self->no == NULL)
	{
		PyErr_SetString(PyExc_AttributeError, "no");
	}
	if (self->name == NULL)
	{
		PyErr_SetString(PyExc_AttributeError, "name");
	}

	return PyUnicode_FromFormat("%S: %S", self->no, self->name);
}

static PyObject *aqbanking_Account_set_callbackLog(aqbanking_Account* self, PyObject *args)
{
	PyObject *result = NULL;
	PyObject *temp;

	if (PyArg_ParseTuple(args, "O:set_callbackLog", &temp)) {
		if (!PyCallable_Check(temp)) {
			PyErr_SetString(PyExc_TypeError, "parameter must be callable");
			return NULL;
		}
		Py_XINCREF(temp);         /* Add a reference to new callback */
		Py_XDECREF(self->aqh->callbackLog);  /* Dispose of previous callback */
		self->aqh->callbackLog = temp;       /* Remember new callback */
		/* Boilerplate to return "None" */
		Py_INCREF(Py_None);
		result = Py_None;
	}
	return result;
}

static PyObject *aqbanking_Account_set_callbackPassword(aqbanking_Account* self, PyObject *args)
{
	PyObject *result = NULL;
	PyObject *temp;

	if (PyArg_ParseTuple(args, "O:set_callbackPassword", &temp)) {
		if (!PyCallable_Check(temp)) {
			PyErr_SetString(PyExc_TypeError, "parameter must be callable");
			return NULL;
		}
		Py_XINCREF(temp);         /* Add a reference to new callback */
		Py_XDECREF(self->aqh->callbackPassword);  /* Dispose of previous callback */
		self->aqh->callbackPassword = temp;       /* Remember new callback */
		if (self->aqh->callbackPassword == NULL) {
			fprintf(stderr, "%s", "!!but is still invalid!!! \n");
		}
		/* Boilerplate to return "None" */
		Py_INCREF(Py_None);
		result = Py_None;
	}
	return result;
}

static PyObject *aqbanking_Account_balance(aqbanking_Account* self, PyObject *args, PyObject *keywds)
{
	const AB_ACCOUNT_STATUS * status;
	const AB_BALANCE * bal;
	const AB_VALUE *v = 0;
	int rv;
	double balance;
	const char *bank_code = PyUnicode_AsUTF8(self->bank_code);
	const char *account_no = PyUnicode_AsUTF8(self->no);
	AB_ACCOUNT *a;
	AB_JOB *job = 0;
	AB_JOB_LIST2 *jl = 0;
	AB_IMEXPORTER_CONTEXT *ctx = 0;
	AB_IMEXPORTER_ACCOUNTINFO *ai;

	// Valid data set?
	if (self->no == NULL)
	{
		PyErr_SetString(PyExc_AttributeError, "no");
	}
	if (self->bank_code == NULL)
	{
		PyErr_SetString(PyExc_AttributeError, "bank_code");
	}

	// Initialize aqbanking.
	rv = AB_create(self);
	if (rv > 0)
	{
		return NULL;
	}

	// Let us find the account!
	a = AB_Banking_GetAccountByCodeAndNumber(self->ab, bank_code, account_no);
	if (!a)
	{
		PyErr_SetString(AccountNotFound, "Could not find the given account! ");
		return NULL;
	}

	// Create job and execute it.
	ctx = AB_ImExporterContext_new();
	jl = AB_Job_List2_new();
	job = AB_JobGetBalance_new(a);
	AB_Job_List2_PushBack(jl, job);
	rv = AB_Banking_ExecuteJobs(self->ab, jl, ctx);

	if (rv)
	{
		PyErr_SetString(ExecutionFailed, "Could not get the balance!");
		return NULL;
	}

	// With success. No process the result.
	ai = AB_ImExporterContext_GetFirstAccountInfo (ctx);
	status = AB_ImExporterAccountInfo_GetFirstAccountStatus (ai);
	bal = AB_AccountStatus_GetBookedBalance (status);
	v = AB_Balance_GetValue (bal);
	balance = AB_Value_GetValueAsDouble(v);

	// Free jobs.
	AB_Job_List2_free(jl);
	AB_ImExporterContext_free(ctx);

	// Exit aqbanking.
	rv = AB_free(self);
	if (rv > 0)
	{
		return NULL;
	}

	// FIXME: currency!
	return Py_BuildValue("(d,s)", balance, "EUR");
}

static PyObject *aqbanking_Account_transactions(aqbanking_Account* self, PyObject *args, PyObject *kwds)
{
	int rv;
	double tmpDateTime = 0;
	const char *bank_code = PyUnicode_AsUTF8(self->bank_code);
	const char *account_no = PyUnicode_AsUTF8(self->no);
	GWEN_TIME *gwTime;
	const char *dateFrom=NULL, *dateTo=NULL;
	static char *kwlist[] = {"dateFrom", "dateTo", NULL};
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "|ss", kwlist, &dateFrom, &dateTo))
	{
		return NULL;
	}

	AB_ACCOUNT *a;
	AB_JOB *job = 0;
	AB_JOB_LIST2 *jl = 0;
	AB_IMEXPORTER_CONTEXT *ctx = 0;
	AB_IMEXPORTER_ACCOUNTINFO *ai;
	/*aqbanking_Transaction *trans = NULL;*/
	PyObject *transList = PyList_New(0);

	// Valid data set?
	if (self->no == NULL)
	{
		PyErr_SetString(PyExc_AttributeError, "no");
	}
	if (self->bank_code == NULL)
	{
		PyErr_SetString(PyExc_AttributeError, "bank_code");
	}

	// Initialize aqbanking.
	rv = AB_create(self);
	if (rv > 0)
	{
		Py_DECREF(transList);
		return NULL;
	}

	// Let us find the account!
	a = AB_Banking_GetAccountByCodeAndNumber(self->ab, bank_code, account_no);
	if (!a)
	{
		PyErr_SetString(AccountNotFound, "Could not find the given account! ");
		Py_DECREF(transList);
		return NULL;
	}

	// Create job and execute it.
	job = AB_JobGetTransactions_new(a);
	if (dateFrom != NULL)
	{
		gwTime = GWEN_Time_fromString(dateFrom, "YYYYMMDD");
		AB_JobGetTransactions_SetFromTime(job, gwTime);
	}
	if (dateTo != NULL)
	{
		gwTime = GWEN_Time_fromString(dateTo, "YYYYMMDD");
		AB_JobGetTransactions_SetToTime(job, gwTime);
	}
	// Check for availability
	rv = AB_Job_CheckAvailability(job);
	if (rv) {
		PyErr_SetString(ExecutionFailed, "Transaction retrieval is not supported!");
		Py_DECREF(transList);
		return NULL;
	}

	jl = AB_Job_List2_new();
	AB_Job_List2_PushBack(jl, job);
	ctx = AB_ImExporterContext_new();
	rv = AB_Banking_ExecuteJobs(self->ab, jl, ctx);

	if (rv)
	{
		PyErr_SetString(ExecutionFailed, "Could not retrieve transactions!");
		Py_DECREF(transList);
		return NULL;
	}

	// With success. No process the result.
	ai = AB_ImExporterContext_GetFirstAccountInfo (ctx);
	while(ai)
	{
		const AB_TRANSACTION *t;
		
		t=AB_ImExporterAccountInfo_GetFirstTransaction(ai);
		while(t) {
			const AB_VALUE *v;
			AB_TRANSACTION_STATUS state;

			v=AB_Transaction_GetValue(t);
			if (v) {
				const GWEN_STRINGLIST *sl;
				const GWEN_TIME *tdtime;
				const char *purpose;
				aqbanking_Transaction *trans = (aqbanking_Transaction*) _PyObject_New(&aqbanking_TransactionType);
				trans = (aqbanking_Transaction*) PyObject_Init((PyObject *)trans, &aqbanking_TransactionType);
				//trans = PyObject_CallObject((PyObject *)&aqbanking_TransactionType, NULL);

				/* The purpose (memo field) might contain multiple lines.
				 * Therefore AqBanking stores the purpose in a string list
				 * of which the first entry is used in this tutorial */
				sl = AB_Transaction_GetPurpose(t);
				if (sl)
				{
					purpose = GWEN_StringList_FirstString(sl);
					if (purpose == NULL) {
						purpose = "";
					}
				}
				else
				{
					purpose = "";
				}

				fprintf(stderr, "[%-10s/%-10s][%-10s/%-10s] %-32s (%.2f %s)\n",
						AB_Transaction_GetRemoteIban(t),
						AB_Transaction_GetRemoteBic(t),
						AB_Transaction_GetRemoteAccountNumber(t),
						AB_Transaction_GetRemoteBankCode(t),
						purpose,
						AB_Value_GetValueAsDouble(v),
						AB_Value_GetCurrency(v)
				);

				tdtime = AB_Transaction_GetDate(t);
				tmpDateTime = PyLong_AsDouble(PyLong_FromSize_t(GWEN_Time_Seconds(tdtime)));
				// FIXME: later PyDateTime_FromTimestamp??
				trans->date = PyDateTime_FromTimestamp(Py_BuildValue("(O)", PyFloat_FromDouble(tmpDateTime)));
				trans->purpose = PyUnicode_FromString(purpose);

				// Local user
				if (AB_Transaction_GetLocalAccountNumber(t) == NULL) {
					trans->localAccount = Py_None;
					Py_INCREF(Py_None);
				} else {
					trans->localAccount = PyUnicode_FromString(AB_Transaction_GetLocalAccountNumber(t));
				}
				if (AB_Transaction_GetLocalBankCode(t) == NULL) {
					trans->localBank = Py_None;
					Py_INCREF(Py_None);
				} else {
					trans->localBank = PyUnicode_FromString(AB_Transaction_GetLocalBankCode(t));
				}
				if (AB_Transaction_GetLocalIban(t) == NULL) {
					trans->localIban = Py_None;
					Py_INCREF(Py_None);
					trans->localBic = Py_None;
					Py_INCREF(Py_None);
				} else {
					trans->localIban = PyUnicode_FromString(AB_Transaction_GetLocalIban(t));
					trans->localBic = PyUnicode_FromString(AB_Transaction_GetLocalBic(t));
				}

				// Remote user
				if (AB_Transaction_GetRemoteAccountNumber(t) == NULL) {
					trans->remoteAccount = Py_None;
					Py_INCREF(Py_None);
				} else {
					trans->remoteAccount = PyUnicode_FromString(AB_Transaction_GetRemoteAccountNumber(t));
				}
				if (AB_Transaction_GetRemoteBankCode(t) == NULL) {
					trans->remoteBank = Py_None;
					Py_INCREF(Py_None);
				} else {
					trans->remoteBank = PyUnicode_FromString(AB_Transaction_GetRemoteBankCode(t));
				}
				if (AB_Transaction_GetRemoteIban(t) == NULL) {
					trans->remoteIban = Py_None;
					Py_INCREF(Py_None);
					trans->remoteBic = Py_None;
					Py_INCREF(Py_None);
				} else {
					trans->remoteIban = PyUnicode_FromString(AB_Transaction_GetRemoteIban(t));
					trans->remoteBic = PyUnicode_FromString(AB_Transaction_GetRemoteBic(t));
				}

				trans->value = PyFloat_FromDouble(AB_Value_GetValueAsDouble(v));
				trans->currency = PyUnicode_FromString("EUR");
				trans->state = 0;
				state = AB_Transaction_GetStatus(t);
				switch(state)
				{
					case AB_Transaction_StatusUnknown:
						trans->state = -1;
						break;
					case AB_Transaction_StatusNone:
						trans->state = 0;
						break;
					case AB_Transaction_StatusAccepted:
						trans->state = 1;
						break;
					case AB_Transaction_StatusRejected:
						trans->state = 2;
						break;
					case AB_Transaction_StatusPending:
						trans->state = 4;
						break;
					case AB_Transaction_StatusSending:
						trans->state = 8;
						break;
					case AB_Transaction_StatusAutoReconciled:
						trans->state = 16;
						break;
					case AB_Transaction_StatusManuallyReconciled:
						trans->state = 32;
						break;
					case AB_Transaction_StatusRevoked:
						trans->state = 64;
						break;
					case AB_Transaction_StatusAborted:
						trans->state = 128;
						break;
				}

				PyList_Append(transList, (PyObject *)trans);
				Py_DECREF(trans);
			}
		  t = AB_ImExporterAccountInfo_GetNextTransaction(ai);
		} 
		ai = AB_ImExporterContext_GetNextAccountInfo(ctx);
	}

	// Free jobs.
	AB_Job_free(job);
	AB_Job_List2_free(jl);
	AB_ImExporterContext_free(ctx);

	// Exit aqbanking.
	rv = AB_free(self);
	if (rv > 0)
	{
		//Py_XDECREF(trans);
		Py_DECREF(transList);
		return NULL;
	}

	return transList;
}

static PyMemberDef aqbanking_Account_members[] = {
	{"no", T_OBJECT_EX, offsetof(aqbanking_Account, no), 0, "Account No."},
	{"name", T_OBJECT_EX, offsetof(aqbanking_Account, name), 0, "Name"},
	{"description", T_OBJECT_EX, offsetof(aqbanking_Account, description), 0, "Type of account (e.g. Kontokorrent)"},
	{"bank_code", T_OBJECT_EX, offsetof(aqbanking_Account, bank_code), 0, "Bank No."},
	{"bank_name", T_OBJECT_EX, offsetof(aqbanking_Account, bank_name), 0, "Name of Bank"},
	{NULL}
};

static PyMethodDef aqbanking_Account_methods[] = {
	/*{"name", (PyCFunction)aqbanking_Account_name, METH_NOARGS,
	 "Return the name, combining the first and last name"
	},*/
	{"balance", (PyCFunction)aqbanking_Account_balance, METH_VARARGS | METH_KEYWORDS, "Get the balance of the account."},
	{"transactions", (PyCFunction)aqbanking_Account_transactions, METH_VARARGS | METH_KEYWORDS, "Get the list of transactions of an account."},
	{"set_callbackLog", (PyCFunction)aqbanking_Account_set_callbackLog, METH_VARARGS, "Adds a callback for the log output."},
	{"set_callbackPassword", (PyCFunction)aqbanking_Account_set_callbackPassword, METH_VARARGS, "Adds a callback to retrieve the password (pin)."},
	{NULL}  /* Sentinel */
};

static PyTypeObject aqbanking_AccountType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"aqbanking.Account", /* tp_name */
	sizeof(aqbanking_Account), /* tp_basicsize */
	0, /* tp_itemsize */
	(destructor)aqbanking_Account_dealloc, /* tp_dealloc */
	0, /* tp_print */
	0, /* tp_getattr */
	0, /* tp_setattr */
	0, /* tp_reserved */
	0, /* tp_repr */
	0, /* tp_as_number */
	0, /* tp_as_sequence */
	0, /* tp_as_mapping */
	0, /* tp_hash  */
	0, /* tp_call */
	0, /* tp_str */
	0, /* tp_getattro */
	0, /* tp_setattro */
	0, /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"Account", /* tp_doc */
	0, /* tp_traverse */
	0, /* tp_clear */
	0, /* tp_richcompare */
	0, /* tp_weaklistoffset */
	0, /* tp_iter */
	0, /* tp_iternext */
	aqbanking_Account_methods, /* tp_methods */
	aqbanking_Account_members, /* tp_members */
	0, /* tp_getset */
	0, /* tp_base */
	0, /* tp_dict */
	0, /* tp_descr_get */
	0, /* tp_descr_set */
	0, /* tp_dictoffset */
	(initproc)aqbanking_Account_init, /* tp_init */
	0, /* tp_alloc */
	aqbanking_Account_New, /* tp_new */
};

static PyObject * aqbanking_listacc(PyObject *self, PyObject *args)
{
	int rv;
	AB_ACCOUNT_LIST2 *accs;
	// List of accounts => to return.
	PyObject *accountList;
	aqbanking_Account *account;
	accountList = PyList_New(0);

	// Initialize aqbanking.
	rv = AB_create(NULL);
	if (rv > 0)
	{
		return NULL;
	}

	/* Get a list of accounts which are known to AqBanking.
	 * There are some pecularities about the list returned:
	 * The list itself is owned by the caller (who must call
	 * AB_Account_List2_free() as we do below), but the elements of that
	 * list (->the accounts) are still owned by AqBanking.
	 * Therefore you MUST NOT free any of the accounts within the list returned.
	 * This also rules out calling AB_Account_List2_freeAll() which not only
	 * frees the list itself but also frees all its elements.
	 *
	 * The rest of this tutorial shows how lists are generally used by
	 * AqBanking.
	 */
	accs = AB_Banking_GetAccounts(ab);
	if (accs) {
		AB_ACCOUNT_LIST2_ITERATOR *it;

		/* List2's are traversed using iterators. An iterator is an object
		 * which points to a single element of a list.
		 * If the list is empty NULL is returned.
		 */
		it=AB_Account_List2_First(accs);
		if (it) {
			AB_ACCOUNT *a;

			/* this function returns a pointer to the element of the list to
			 * which the iterator currently points to */
			a=AB_Account_List2Iterator_Data(it);
			while(a) {
				AB_PROVIDER *pro;
				account = (aqbanking_Account*) _PyObject_New(&aqbanking_AccountType);
				account = (aqbanking_Account*) PyObject_Init((PyObject *)account, &aqbanking_AccountType);

				/* every account is assigned to a backend (sometimes called provider)
				 * which actually performs online banking tasks. We get a pointer
				 * to that provider/backend with this call to show its name in our
				 * example.*/
				pro = AB_Account_GetProvider(a);
				// Populate the object.
				account->no = PyUnicode_FromString(AB_Account_GetAccountNumber(a));
				account->name = PyUnicode_FromString(AB_Account_GetAccountName(a));
				account->description = PyUnicode_FromString(AB_Provider_GetName(pro));
				account->bank_code = PyUnicode_FromString(AB_Account_GetBankCode(a));
				account->bank_name = PyUnicode_FromString(AB_Account_GetBankName(a));
				PyList_Append(accountList, (PyObject *)account);
				Py_DECREF(account);

				/* this function lets the iterator advance to the next element in
				 * the list, so a following call to AB_Account_List2Iterator_Data()
				 * would return a pointer to the next element.
				 * This function also returns a pointer to the next element of the
				 * list. If there is no next element then NULL is returned. */
				a = AB_Account_List2Iterator_Next(it);
			}
			/* the iterator must be freed after using it */
			AB_Account_List2Iterator_free(it);
		}
		/* as discussed the list itself is only a container which has to be freed
		 * after use. This explicitly does not free any of the elements in that
		 * list, and it shouldn't because AqBanking still is the owner of the
		 * accounts */
		AB_Account_List2_free(accs);
	}

	// Exit aqbanking.
	rv = AB_free(NULL);
	if (rv > 0)
	{
		Py_DECREF(account);
		Py_DECREF(accountList);
		return NULL;
	}

	return accountList;
}

static PyObject *aqbanking_chkiban(PyObject *self, PyObject *args)
{
	int res;
	int rv;
	// List of accounts => to return.
	const char *iban;

	if (!PyArg_ParseTuple(args, "s", &iban))
		return NULL;
	
	// Initialize aqbanking.
	rv = AB_create(NULL);
	if (rv > 0)
	{
		return NULL;
	}

	res = AB_Banking_CheckIban(iban);

	// Exit aqbanking.
	rv = AB_free(NULL);
	if (rv > 0)
	{
		return NULL;
	}

	if (res == 0) {
		return PyBool_FromLong(1);
	} else {
		return PyBool_FromLong(0);
	}
}

static PyMethodDef AqBankingMethods[] = {
	{"listacc", aqbanking_listacc, METH_VARARGS, "Get a list of accounts"},
	{"chkiban", aqbanking_chkiban, METH_VARARGS, "Validates an IBAN"},

	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef aqbankingmodule = {
	PyModuleDef_HEAD_INIT,
	"aqbanking",
	"This is an API to the AqBanking software.",
	-1,
	AqBankingMethods
};

PyMODINIT_FUNC
PyInit_aqbanking(void)
{
	PyObject *m;

	// First initialize the types
	aqbanking_AccountType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&aqbanking_AccountType) < 0)
		return NULL;

	aqbanking_TransactionType.tp_new = PyType_GenericNew;
	if (PyType_Ready(&aqbanking_TransactionType) < 0)
		return NULL;

	// And then the module. 
	m = PyModule_Create(&aqbankingmodule);
	if (m == NULL)
		return NULL;

	// Exceptions 
	AqBankingInitializeError = PyErr_NewException("aqbanking.AqBankingInitializeError", NULL, NULL);
	Py_INCREF(AqBankingInitializeError);
	PyModule_AddObject(m, "AqBankingInitializeError", AqBankingInitializeError);
	AqBankingDeInitializeError = PyErr_NewException("aqbanking.AqBankingDeInitializeError", NULL, NULL);
	Py_INCREF(AqBankingDeInitializeError);
	PyModule_AddObject(m, "AqBankingDeInitializeError", AqBankingDeInitializeError);
	AccountNotFound = PyErr_NewException("aqbanking.AccountNotFound", NULL, NULL);
	Py_INCREF(AccountNotFound);
	PyModule_AddObject(m, "AccountNotFound", AccountNotFound);
	ExecutionFailed = PyErr_NewException("aqbanking.ExecutionFailed", NULL, NULL);
	Py_INCREF(ExecutionFailed);
	PyModule_AddObject(m, "ExecutionFailed", ExecutionFailed);

	// Some types
	Py_INCREF(&aqbanking_AccountType);
	PyModule_AddObject(m, "Account", (PyObject *)&aqbanking_AccountType);
	Py_INCREF(&aqbanking_TransactionType);
	PyModule_AddObject(m, "Transaction", (PyObject *)&aqbanking_TransactionType);

	// Initialize PyDateTime
	PyDateTime_IMPORT;

	aqh = new PyAqHandler();

	return m;
}
 
