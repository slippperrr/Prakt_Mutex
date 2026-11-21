#include <iostream>
#include <fstream>
#include <Windows.h>
using namespace std;

int maxBalance = 1000000;

struct BankAccount
{
	int accountNumber;
	int balance;
};

bool CheckData(BankAccount& acc)
{
	if (acc.accountNumber != 10001) return false;
	if (acc.balance < 0 || acc.balance > maxBalance) return false;

	return true;
}

bool NewFile(BankAccount& acc)
{
	ofstream acc_file("account.bin", ios::binary);

	if (!acc_file) return false;

	acc = { 10001, 10000 };

	acc_file.write(reinterpret_cast<const char*>(&acc), sizeof(BankAccount));

	bool er = acc_file.good();

	acc_file.close();

	return er;
}

bool ReadFile(BankAccount& acc)
{
	ifstream acc_file("account.bin", ios::binary);

	if (!acc_file)
	{
		return NewFile(acc);
	}

	acc_file.read(reinterpret_cast<char*>(&acc), sizeof(BankAccount));

	if (!acc_file.good())
	{
		acc_file.close();
		return false;
	}

	acc_file.close();

	return CheckData(acc);
}

bool SaveAcc(BankAccount& acc)
{
	ofstream acc_file( "account.bin", ios::binary | ios::trunc);

	if (!acc_file) return false;

	acc_file.write(reinterpret_cast<const char*>(&acc), sizeof(BankAccount));

	bool er = acc_file.good();

	acc_file.close();

	return er;
}


// СОЗДАНИЕ МЬЮТЕКСА
HANDLE cbMutex()
{
	HANDLE hMutex = CreateMutex(NULL, FALSE, L"Local\\TurchanovaMutex");

	if (!hMutex) cout << "Мьютекс не создан!\n";

	return hMutex;
}

// ЗАХВАТ МЬЮТЕКСА
bool ZMutex(HANDLE hMutex)
{

	DWORD result = WaitForSingleObject(hMutex, 3000);

	if (result == WAIT_OBJECT_0) return true;

	if (result == WAIT_ABANDONED)
	{

		cout << "Мьютекс был освобожден срочно.\n";

		return true;
	}

	if (result == WAIT_TIMEOUT) cout << "Время ожидания мьютекса истекло!\n";

	if (result == WAIT_FAILED) cout << "Ошибка ожидания мьютекса!\n";

	return false;
}


// ЗАПИСЬ В ЛОГ
void WriteLog( DWORD pid, int startBalance, int oper, int sum, int endBalance, int status)
{
	ofstream log_file("operations.log", ios::app);

	if (!log_file)
	{
		cout << "Ошибка открытия файла лога!\n";
		return;
	}

	log_file << "PID | Начальный баланс | Операция | Сумма | Конечный баланс | Статус\n" << endl;
	
	log_file << pid << " | " << startBalance << " | ";

	if (oper == 1) log_file << "Пополнение";
	else log_file << "Снятие";

	log_file << " | " << sum << " | " << endBalance << " | ";

	if (status == 1) log_file << "Успешно";
	else log_file << "Ошибка";

	log_file << endl;

	log_file.close();
}


void ShowBalance(HANDLE hMutex)
{

	if (!ZMutex(hMutex)) return;

	BankAccount acc;

	if (!ReadFile(acc))
	{
		cout << "Данные банковского счёта повреждены!\n";

		ReleaseMutex(hMutex);
		return;
	}

	cout << "\nНомер счёта: " << acc.accountNumber << endl;
	cout << "Текущий баланс: " << acc.balance << " руб.\n";

	ReleaseMutex(hMutex);
}


void Money(HANDLE hMutex, int money)
{
	if (!ZMutex(hMutex)) return;

	BankAccount acc;

	if (!ReadFile(acc))
	{
		cout << "Данные банковского счёта повреждены!\n";

		ReleaseMutex(hMutex);
		return;
	}

	int startBalance = acc.balance;
	acc.balance += money;

	if (money <= 0)
	{
		cout << "Сумма должна быть положительной!\n";

		WriteLog(GetCurrentProcessId(), startBalance, 1, money, acc.balance, 0);

		ReleaseMutex(hMutex);
		return;
	}

	if (money > maxBalance - acc.balance)
	{
		cout << "Превышен максимальный баланс!\n";

		WriteLog(GetCurrentProcessId(), startBalance, 1, money, acc.balance, 0);

		ReleaseMutex(hMutex);
		return;
	}


	if (!SaveAcc(acc))
	{
		cout << "Ошибка сохранения счёта!\n";

		WriteLog(GetCurrentProcessId(), startBalance, 1, money, acc.balance, 0);

		ReleaseMutex(hMutex);
		return;
	}

	cout << "Счёт успешно пополнен!\n";
	cout << "Статус: успешно\n";
	cout << "Конечный баланс: " << acc.balance << " руб.\n";

	WriteLog(GetCurrentProcessId(), startBalance, 1, money, acc.balance, 1);

	ReleaseMutex(hMutex);
}

void Withdraw(HANDLE hMutex, int money)
{
	if (!ZMutex(hMutex)) return;

	BankAccount acc;

	if (!ReadFile(acc))
	{
		cout << "Данные банковского счёта повреждены!\n";

		ReleaseMutex(hMutex);
		return;
	}

	int startBalance = acc.balance;
	acc.balance -= money;


	if (money <= 0)
	{
		cout << "Сумма должна быть положительной!\n";

		WriteLog(GetCurrentProcessId(), startBalance, 2, money, acc.balance, 0);

		ReleaseMutex(hMutex);
		return;
	}

	if (money > acc.balance)
	{
		cout << "Недостаточно средств!\n";

		WriteLog(GetCurrentProcessId(),startBalance, 2, money, acc.balance, 0);

		ReleaseMutex(hMutex);
		return;
	}

	if (!SaveAcc(acc))
	{
		cout << "Ошибка сохранения счёта!\n";

		WriteLog(GetCurrentProcessId(), startBalance, 2, money, acc.balance, 0);

		ReleaseMutex(hMutex);
		return;
	}

	cout << "Деньги успешно сняты!\n";
	cout << "Статус: успешно\n";
	cout << "Конечный баланс: " << acc.balance << " руб.\n";

	WriteLog(GetCurrentProcessId(), startBalance, 2, money, acc.balance, 1);	

	ReleaseMutex(hMutex);
}


void Operations20(HANDLE hMutex)
{
	for (int i = 0; i < 20; i++)
	{
		int amount = 100 + rand() % 901;

		cout << "\nОперация " << i + 1 << " из 20\n";

		if ((rand() + i) % 2 == 0) Money(hMutex, amount);
		else
			Withdraw(hMutex, amount);

		if (i != 19)
		{
			int delay = 100 + rand() % 401;

			Sleep(delay);
		}
	}

	cout << "\nАвтоматические операции завершены!\n";
}


int main()
{
	setlocale(NULL, "Russian");

	HANDLE hMutex = cbMutex();

	if (!hMutex) return 1;

	cout << "БАНКОВСКИЙ ТЕРМИНАЛ\n";

	cout << "PID процесса: " << GetCurrentProcessId() << endl;

	int choice;

	do
	{
		cout << "1. Посмотреть текущий баланс\n";
		cout << "2. Пополнение счёта\n";
		cout << "3. Снятие денежных средств\n";
		cout << "4. Выполнить 20 случайных операций\n";
		cout << "5. Выход\n";

		cout << "Ваш выбор: ";

		if (!(cin >> choice))
		{
			cin.clear();
			cin.ignore();

			cout << "Некорректный ввод!\n";
		}

		switch (choice)
		{
		case 1:
		{
			ShowBalance(hMutex);
			break;
		}

		case 2:
		{
			int money;

			cout << "Введите сумму пополнения: ";

			if (cin >> money) Money(hMutex, money);
			else
			{
				cin.clear();
				cin.ignore();

				cout << "Некорректная сумма!\n";
			}

			break;
		}

		case 3:
		{
			int money;

			cout << "Введите сумму снятия: ";

			if (cin >> money) Withdraw(hMutex, money);
			else
			{
				cin.clear();
				cin.ignore();

				cout << "Некорректная сумма!\n";
			}

			break;
		}

		case 4:
		{
			Operations20(hMutex);
			break;
		}

		case 5:
		{
			cout << "Завершение работы программы...\n";
			break;
		}

		default:
		{
			cout << "Неверный ввод, попробуйте ещё раз!\n";
			break;
		}
		}

	} while (choice != 5);

	CloseHandle(hMutex);

	return 0;
}