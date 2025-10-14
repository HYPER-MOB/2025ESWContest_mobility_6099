#pragma once
#include "common.hpp"
#include <string>


/**
* NFCModule
* - NFC ���� �ʱ�ȭ �� ���� ������ ����.
* - �±� ���� �� UDS(JSON) `{ "type":"nfc_ok", "uid":"...", "token":"..." }` ����.
* - ����� ���� ����(�ֱ��� ��¥ �̺�Ʈ); ���� ������ pcsc-lite �Ǵ� libnfc ���� ����.
*/
class NFCModule {
public:
	bool init(); ///< ����̹�/���� �ڵ� �غ� (�Ǳ��� ��)
	bool pollStart(); ///< ���� ���� ���� (�±� ���� �� �̺�Ʈ ����)
	void pollStop(); ///< ���� ���� ����


	NFCModule(const std::string& uds_path) : uds_path_(uds_path) {}
private:
	std::string uds_path_; ///< �̺�Ʈ ���� ��� UDS ���
};