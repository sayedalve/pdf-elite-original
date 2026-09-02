package pdfelite.software.saas.procurement.repository;

import java.util.List;

import org.springframework.data.jpa.repository.JpaRepository;

import pdfelite.software.saas.procurement.model.ProcurementQuote;

public interface ProcurementQuoteRepository extends JpaRepository<ProcurementQuote, Long> {

    List<ProcurementQuote> findByDealIdOrderByCreatedAtDesc(Long dealId);
}
