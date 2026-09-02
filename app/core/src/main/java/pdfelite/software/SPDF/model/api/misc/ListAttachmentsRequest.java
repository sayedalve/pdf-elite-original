package pdfelite.software.SPDF.model.api.misc;

import lombok.Data;
import lombok.EqualsAndHashCode;

import pdfelite.software.common.model.api.PDFFile;

@Data
@EqualsAndHashCode(callSuper = true)
public class ListAttachmentsRequest extends PDFFile {}
