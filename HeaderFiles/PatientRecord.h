#pragma once
#include <string>

using namespace std;


class PatientRecord {
private:
    int entryDate, age;
    string recordID, patientFirstName, patientLastName, country, diseaseID;

public:
    int exitDate;

    PatientRecord(string id, string firstName, string lastName, string disease, string region, int patientAge, int entry, int exit);
    string id();       // return ID
    int entry();    // return Entry Date
    int getAge();
    bool compare(PatientRecord *x);     // compare patient record with x (x can be dummy record)
    string firstName();
    string lastName();
    string getCountry();
    string disease();
};

