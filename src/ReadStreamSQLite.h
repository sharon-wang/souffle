/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ReadStreamSQLite.h
 *
 ***********************************************************************/

#pragma once

#include "RamTypes.h"
#include "ReadStream.h"
#include "RecordTable.h"
#include "SymbolTable.h"
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include <sqlite3.h>

namespace souffle {

class ReadStreamSQLite : public ReadStream {
public:
    ReadStreamSQLite(const RWOperation& rwOperation, SymbolTable& symbolTable, RecordTable& recordTable)
            : ReadStream(rwOperation, symbolTable, recordTable), dbFilename(rwOperation.get("dbname")),
              relationName(rwOperation.get("name")) {
        openDB();
        checkTableExists();
        prepareSelectStatement();
    }

    ~ReadStreamSQLite() override {
        sqlite3_finalize(selectStatement);
        sqlite3_close(db);
    }

protected:
    /**
     * Read and return the next tuple.
     *
     * Returns nullptr if no tuple was readable.
     * @return
     */
    std::unique_ptr<RamDomain[]> readNextTuple() override {
        if (sqlite3_step(selectStatement) != SQLITE_ROW) {
            return nullptr;
        }

        std::unique_ptr<RamDomain[]> tuple = std::make_unique<RamDomain[]>(arity + auxiliaryArity);

        uint32_t column;
        for (column = 0; column < arity; column++) {
            std::string element(reinterpret_cast<const char*>(sqlite3_column_text(selectStatement, column)));

            if (element.empty()) {
                element = "n/a";
            }

            try {
                switch (typeAttributes.at(column)[0]) {
                    case 's':
                        tuple[column] = symbolTable.unsafeLookup(element);
                        break;
                    case 'i':
                    case 'u':
                    case 'f':
                    case 'r':
                        tuple[column] = RamSignedFromString(element);
                        break;
                    default:
                        assert(false && "Invalid type attribute");
                }
            } catch (...) {
                std::stringstream errorMessage;
                errorMessage << "Error converting number in column " << (column) + 1;
                throw std::invalid_argument(errorMessage.str());
            }
        }

        return tuple;
    }

    void executeSQL(const std::string& sql) {
        assert(db && "Database connection is closed");

        char* errorMessage = nullptr;
        /* Execute SQL statement */
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errorMessage);
        if (rc != SQLITE_OK) {
            std::stringstream error;
            error << "SQLite error in sqlite3_exec: " << sqlite3_errmsg(db) << "\n";
            error << "SQL error: " << errorMessage << "\n";
            error << "SQL: " << sql << "\n";
            sqlite3_free(errorMessage);
            throw std::invalid_argument(error.str());
        }
    }

    void throwError(const std::string& message) {
        std::stringstream error;
        error << message << sqlite3_errmsg(db) << "\n";
        throw std::invalid_argument(error.str());
    }

    void prepareSelectStatement() {
        std::stringstream selectSQL;
        selectSQL << "SELECT * FROM '" << relationName << "'";
        const char* tail = nullptr;
        if (sqlite3_prepare_v2(db, selectSQL.str().c_str(), -1, &selectStatement, &tail) != SQLITE_OK) {
            throwError("SQLite error in sqlite3_prepare_v2: ");
        }
    }

    void openDB() {
        if (sqlite3_open(dbFilename.c_str(), &db) != SQLITE_OK) {
            throwError("SQLite error in sqlite3_open: ");
        }
        sqlite3_extended_result_codes(db, 1);
        executeSQL("PRAGMA synchronous = OFF");
        executeSQL("PRAGMA journal_mode = MEMORY");
    }

    void checkTableExists() {
        sqlite3_stmt* tableStatement;
        std::stringstream selectSQL;
        selectSQL << "SELECT count(*) FROM sqlite_master WHERE type IN ('table', 'view') AND ";
        selectSQL << " name = '" << relationName << "';";
        const char* tail = nullptr;

        if (sqlite3_prepare_v2(db, selectSQL.str().c_str(), -1, &tableStatement, &tail) != SQLITE_OK) {
            throwError("SQLite error in sqlite3_prepare_v2: ");
        }

        if (sqlite3_step(tableStatement) == SQLITE_ROW) {
            int count = sqlite3_column_int(tableStatement, 0);
            if (count > 0) {
                sqlite3_finalize(tableStatement);
                return;
            }
        }
        sqlite3_finalize(tableStatement);
        throw std::invalid_argument("Required table or view does not exist for relation " + relationName);
    }
    const std::string& dbFilename;
    const std::string& relationName;
    sqlite3_stmt* selectStatement = nullptr;
    sqlite3* db = nullptr;
};

class ReadSQLiteFactory : public ReadStreamFactory {
public:
    std::unique_ptr<ReadStream> getReader(
            const RWOperation& rwOperation, SymbolTable& symbolTable, RecordTable& recordTable) override {
        return std::make_unique<ReadStreamSQLite>(rwOperation, symbolTable, recordTable);
    }

    const std::string& getName() const override {
        static const std::string name = "sqlite";
        return name;
    }
    ~ReadSQLiteFactory() override = default;
};

} /* namespace souffle */
