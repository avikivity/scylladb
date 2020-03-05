#pragma once

/*
 * Copyright (C) 2014 ScyllaDB
 */

/*
 * This file is part of Scylla.
 *
 * Scylla is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Scylla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Scylla.  If not, see <http://www.gnu.org/licenses/>.
 */

// The following is a redesigned subset of Java's DataOutput,
// DataOutputStream, DataInput, DataInputStream, etc. It allows serializing
// several primitive types (e.g., integer, string, etc.) to an object which
// is only capable of write()ing a single byte (write(char)) or an array of
// bytes (write(char *, int)), and deserializing the same data from an object
// with a char read() interface.
//
// The format of this serialization is identical to the format used by
// Java's DataOutputStream class. This is important to allow us communicate
// with nodes running Java version of the code.
//
// We only support the subset actually used in Cassandra, and the subset
// that is reversible, i.e., can be read back by data_input. For example,
// we only support DataOutput.writeUTF(string) and not
// DataOutput.writeChars(string) - because the latter does not include
// the length, which is necessary for reading the string back.

