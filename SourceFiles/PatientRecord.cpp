#include "../HeaderFiles/PatientRecord.h"


PatientRecord::PatientRecord(string id, string firstName, string lastName, string disease, string region, int patientAge, int entry, int exit) {
    recordID = id;
    patientFirstName = firstName;
    patientLastName = lastName;
    diseaseID = disease;
    country = region;
    entryDate = entry;
    exitDate = exit;
    age = patientAge;
}

string PatientRecord::id() {
    return recordID;
}

int PatientRecord::entry() {
    return entryDate;
}

string PatientRecord::firstName() {
    return patientFirstName;
}

string PatientRecord::lastName() {
    return patientLastName;
}

string PatientRecord::getCountry() {
    return country;
}

string PatientRecord::disease() {
    return diseaseID;
}

int PatientRecord::getAge() {
    return age;
}


bool inAgeRange(int age, int range) {
    return (range >= 60) ? (age > 60) : (age - range <= 10 && age - range > 0);
}
// Compare with x
// if x has entry = -1 or "" ignore entry
bool PatientRecord::compare(PatientRecord *x) {
    if(x == NULL)
        return true;
    
    if(x->id() != "" && x->id() != recordID)
        return false;
    if(x->firstName() != "" && x->firstName() != patientFirstName)
        return false;
    if(x->lastName() != "" && x->lastName() != patientLastName)
        return false;
    if(x->disease() != "" && x->disease() != diseaseID)
        return false;
    if(x->getCountry() != "" && x->getCountry() != country)
        return false;
    if(x->entry() != -1 && x->entry() != entryDate)
        return false;
    if(x->exitDate != -1 && x->exitDate != exitDate)
        return false;
    if(x->getAge() != -1 && !inAgeRange(age, x->getAge()))
        return false;
    
    return true;
}