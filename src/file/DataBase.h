#pragma once
#include "bpt_exception.h"
#include <filesystem>
#include <fstream>
#include <string>

namespace kupi {
template<typename Type, bool isTrash = false>
class DataBase {
public:
	using emptyHook = void(std::fstream &);
	explicit DataBase(std::string const &filename, emptyHook *hook = nullptr);
	~DataBase() {
		file.close();
		trash.close();
	}
	/**
     * @brief Insert a Data to a proper place
     * @return the 1-indexed id of the place where it's put
     */
	int insert(Type const &);
	/**
     * @brief Read a block of Data from file
     * @param id 1-indexed
     */
	Type read(int id);
	void read(int id, Type &res);
	/**
     * @brief Write a Data to file
     * @param id 1-indexed
     */
	void write(int id, Type const &);
	/**
     * @param id 1-indexed
     * @attention just remove the id to recycle, do not clear the data on disk
     */
	void erase(int id);
	/**
     * @return the number of datas currently being used
     */
	int size();
	int capacity();

protected:
	static constexpr int BLOCK_SIZE = (sizeof(Type) + 4095) / 4096 * 4096;
	// static constexpr int BLOCK_SIZE = sizeof(Type);
	/**
     * @return the 1-indexed id of a usable place
     */
	int newId();

protected:
	std::string filename;
	std::fstream file;
	std::fstream trash;
};

template<typename Type, bool isTrash>
DataBase<Type, isTrash>::DataBase(const std::string &filename, emptyHook *hook)
	: filename(filename) {
	std::filesystem::path path(filename);
	if (path.has_parent_path()) {
		std::filesystem::create_directories(path.parent_path());
	}
	file.open(filename, std::ios::app);
	if (!file)
		throw bpt_exception("file: failed to open file.");
	if (file.tellp() == 0 && hook) hook(file);
	file.close();
	file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

	if constexpr (isTrash) {
		trash.open(filename + ".trash", std::ios::binary | std::ios::app);
		if (trash.tellp() == 0) {
			int a = 0;
			trash.write(reinterpret_cast<char *>(&a), 4);
		}
		trash.close();
		trash.open(filename + ".trash", std::ios::in | std::ios::out | std::ios::binary);
	}
}

template<typename Type, bool isTrash>
int DataBase<Type, isTrash>::insert(const Type &data) {
	int id = newId();
	file.seekp((id - 1) * BLOCK_SIZE);
	file.write(reinterpret_cast<char const *>(&data), sizeof(Type));
	return id;
}

template<typename Type, bool isTrash>
Type DataBase<Type, isTrash>::read(int id) {
	Type ret;
	read(id, ret);
	return ret;
}

template<typename Type, bool isTrash>
void DataBase<Type, isTrash>::read(int id, Type &res) {
	file.seekg((id - 1) * BLOCK_SIZE);
	file.read(reinterpret_cast<char *>(&res), sizeof(Type));
}

template<typename Type, bool isTrash>
void DataBase<Type, isTrash>::write(int id, const Type &data) {
	file.seekp((id - 1) * BLOCK_SIZE);
	file.write(reinterpret_cast<char const *>(&data), sizeof(Type));
}

template<typename Type, bool isTrash>
int DataBase<Type, isTrash>::newId() {
	if constexpr (!isTrash) return size() + 1;
	int id = 0;
	trash.seekg(0);
	int count_trash = 0;
	trash.read(reinterpret_cast<char *>(&count_trash), sizeof(int));
	if (count_trash > 0) {
		trash.seekg(count_trash * sizeof(int));
		trash.read(reinterpret_cast<char *>(&id), sizeof(int));
		--count_trash;
		trash.seekg(0);
		trash.write(reinterpret_cast<char *>(&count_trash), sizeof(int));
	}
	else
		id = size() + 1;
	return id;
}

template<typename Type, bool isTrash>
void DataBase<Type, isTrash>::erase(int id) {
	if constexpr (!isTrash) return;
	trash.seekg(0);
	int count_trash = 0;
	trash.read(reinterpret_cast<char *>(&count_trash), sizeof(int));
	++count_trash;
	trash.seekg(count_trash * sizeof(int));
	trash.write(reinterpret_cast<char *>(&id), sizeof(int));
	trash.seekg(0);
	trash.write(reinterpret_cast<char *>(&count_trash), sizeof(int));
}

template<typename Type, bool isTrash>
int DataBase<Type, isTrash>::size() {
	int count_blocks = capacity(), count_trash = 0;
	trash.seekg(0);
	trash.read(reinterpret_cast<char *>(&count_trash), sizeof(int));
	return count_blocks - count_trash;
}

template<typename Type, bool isTrash>
int DataBase<Type, isTrash>::capacity() {
	file.seekg(0, std::ios::end);
	return (size_t(file.tellg()) + BLOCK_SIZE - 1) / BLOCK_SIZE;
}

}// namespace kupi