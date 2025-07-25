#pragma once

#include <Storages/MergeTree/MergeTreeDataPartWriterOnDisk.h>
#include <Formats/MarkInCompressedFile.h>


namespace DB
{

struct StreamNameAndMark
{
    String stream_name;
    MarkInCompressedFile mark;
};

using StreamsWithMarks = std::vector<StreamNameAndMark>;
using ColumnNameToMark = std::unordered_map<String, StreamsWithMarks>;

/// Writes data part in wide format.
class MergeTreeDataPartWriterWide : public MergeTreeDataPartWriterOnDisk
{
    using Base = MergeTreeDataPartWriterOnDisk;

public:
    MergeTreeDataPartWriterWide(
        const String & data_part_name_,
        const String & logger_name_,
        const SerializationByName & serializations_,
        MutableDataPartStoragePtr data_part_storage_,
        const MergeTreeIndexGranularityInfo & index_granularity_info_,
        const MergeTreeSettingsPtr & storage_settings_,
        const NamesAndTypesList & columns_list,
        const StorageMetadataPtr & metadata_snapshot,
        const VirtualsDescriptionPtr & virtual_columns_,
        const std::vector<MergeTreeIndexPtr> & indices_to_recalc,
        const ColumnsStatistics & stats_to_recalc_,
        const String & marks_file_extension,
        const CompressionCodecPtr & default_codec,
        const MergeTreeWriterSettings & settings,
        MergeTreeIndexGranularityPtr index_granularity_);

    void write(const Block & block, const IColumnPermutation * permutation) override;

    void fillChecksums(MergeTreeDataPartChecksums & checksums, NameSet & checksums_to_remove) final;

    void finish(bool sync) final;
    void cancel() noexcept override;

    size_t getNumberOfOpenStreams() const override { return column_streams.size(); }

private:
    /// Finish serialization of data: write final mark if required and compute checksums
    /// Also validate written data in debug mode
    void fillDataChecksums(MergeTreeDataPartChecksums & checksums, NameSet & checksums_to_remove);
    void finishDataSerialization(bool sync);

    /// Write data of one column.
    /// Return how many marks were written and
    /// how many rows were written for last mark
    void writeColumn(
        const NameAndTypePair & name_and_type,
        const IColumn & column,
        WrittenOffsetColumns & offset_columns,
        const Granules & granules);

    /// Write single granule of one column.
    void writeSingleGranule(
        const NameAndTypePair & name_and_type,
        const IColumn & column,
        WrittenOffsetColumns & offset_columns,
        ISerialization::SerializeBinaryBulkStatePtr & serialization_state,
        ISerialization::SerializeBinaryBulkSettings & serialize_settings,
        const Granule & granule);

    /// Take offsets from column and return as MarkInCompressed file with stream name
    StreamsWithMarks getCurrentMarksForColumn(
        const NameAndTypePair & name_and_type,
        const ColumnPtr & column_sample,
        WrittenOffsetColumns & offset_columns);

    /// Write mark to disk using stream and rows count
    void flushMarkToFile(
        const StreamNameAndMark & stream_with_mark,
        size_t rows_in_mark);

    /// Write mark for column taking offsets from column stream
    void writeSingleMark(
        const NameAndTypePair & name_and_type,
        WrittenOffsetColumns & offset_columns,
        size_t number_of_rows);

    void writeFinalMark(
        const NameAndTypePair & name_and_type,
        WrittenOffsetColumns & offset_columns);

    void addStreams(
        const NameAndTypePair & name_and_type,
        const ColumnPtr & column,
        const ASTPtr & effective_codec_desc) override;

    /// Method for self check (used in debug-build only). Checks that written
    /// data and corresponding marks are consistent. Otherwise throws logical
    /// errors.
    void validateColumnOfFixedSize(const NameAndTypePair & name_type);

    void fillIndexGranularity(size_t index_granularity_for_block, size_t rows_in_block) override;

    /// Use information from just written granules to shift current mark
    /// in our index_granularity array.
    void shiftCurrentMark(const Granules & granules_written);

    /// Change rows in the last mark in index_granularity to new_rows_in_last_mark.
    /// Flush all marks from last_non_written_marks to disk and increment current mark if already written rows
    /// (rows_written_in_last_granule) equal to new_rows_in_last_mark.
    ///
    /// This function used when blocks change granularity drastically and we have unfinished mark.
    /// Also useful to have exact amount of rows in last (non-final) mark.
    void adjustLastMarkIfNeedAndFlushToDisk(size_t new_rows_in_last_mark);

    void initColumnsSubstreamsIfNeeded(const Block & block);

    ISerialization::SerializeBinaryBulkSettings getSerializationSettings() const;

    ISerialization::OutputStreamGetter createStreamGetter(const NameAndTypePair & column, WrittenOffsetColumns & offset_columns) const;
    const String & getStreamName(const NameAndTypePair & column, const ISerialization::SubstreamPath & substream_path) const;

    using SerializationState = ISerialization::SerializeBinaryBulkStatePtr;
    using SerializationStates = std::unordered_map<String, SerializationState>;

    SerializationStates serialization_states;

    using ColumnStreams = std::map<String, StreamPtr>;
    ColumnStreams column_streams;

    /// Some long column names may be replaced to hashes.
    /// Below are mapping from original stream name to actual
    /// stream name (probably hash of the stream) and vice versa.
    std::unordered_map<String, String> full_name_to_stream_name;
    std::unordered_map<String, String> stream_name_to_full_name;

    /// Non written marks to disk (for each column). Waiting until all rows for
    /// this marks will be written to disk.
    using MarksForColumns = std::unordered_map<String, StreamsWithMarks>;
    MarksForColumns last_non_written_marks;

    /// Set of columns to put marks in cache during write.
    NameSet columns_to_load_marks;

    /// How many rows we have already written in the current mark.
    /// More than zero when incoming blocks are smaller then their granularity.
    size_t rows_written_in_last_mark = 0;
};

}
