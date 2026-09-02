package pdfelite.software.proprietary.storage.repository;

import org.springframework.data.jpa.repository.JpaRepository;

import pdfelite.software.proprietary.storage.model.StoredFileBlob;

public interface StoredFileBlobRepository extends JpaRepository<StoredFileBlob, String> {}
