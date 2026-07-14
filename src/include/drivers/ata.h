#ifndef HVOS_DRIVER_ATA_H
#define HVOS_DRIVER_ATA_H

#include <stdint.h>
#include <sys/cdefs.h>
#define MASTER_DRIVE 0xA0
#define SLAVE_DRIVE 0xB0

enum {
    ATA_BUS_PRIMARY_IO = 0x1F0,
    ATA_BUS_SECONDARY_IO = 0x170,
};

enum {
    ATA_DISK_MASTER = 0xA,
    ATA_DISK_SLAVE = 0xB,
};

#define ATA_CTRL_BASE	0x3F6


//Data Register
#define ATA_DATA_REG(bus)		    (bus+0)
//Error Register
#define ATA_ERROR_REG(bus)		    (bus+1)
//Features Register
#define ATA_FEATURES_REG(bus)	    (bus+1)
//Sector Count Register
#define ATA_SECT_CNT_REG(bus)	    (bus+2)
//Sector Number Register (LBAlo)
#define ATA_SECT_NB_REG(bus)		(bus+3)
#define ATA_LBA_LO(bus)			    (bus+3)
//Cylinder Low Register / (LBAmid)
#define ATA_CYL_LO_REG(bus)		    (bus+4)
#define ATA_LBA_MID(bus)			(bus+4)
//Cylinder High Register / (LBAhi)
#define ATA_CYL_HI_REG(bus)		    (bus+5)
#define ATA_LBA_HI(bus)			    (bus+5)
//Drive / Head Register
#define ATA_DRV_HEAD_REG(bus)	    (bus+6)
#define ATA_DRV_SEL_REG(bus)		(bus+6)
//Status Register
#define ATA_STATUS_REG(bus)		    (bus+7)
//Command Register
#define ATA_COMMAND_REG(bus)		(bus+7)


//Alternate Status Register
#define ATA_ATL_STATUS_REG	(ATA_CTRL_BASE + 0)
//Device Control Register
#define ATA_DEV_CTRL_REG	(ATA_CTRL_BASE + 0)
//Drive Address Register
#define ATA_DEV_ADDR_REG	(ATA_CTRL_BASE + 1)


#define ATA_CMD_READ_SECT	0x20
#define ATA_CMD_READ_DMA_EXT 0x25
#define ATA_CMD_WRITE_DMA_EXT 0x35
#define ATA_CMD_WRITE_SECT	0x30
#define ATA_CMD_IDENTIFY	0xEC
typedef union {
	struct {
		uint8_t err: 1;
		uint8_t idx: 1;
		uint8_t corr: 1;
		uint8_t drq: 1;
		uint8_t srv: 1;
		uint8_t df: 1;
		uint8_t rdy: 1;
		uint8_t bsy: 1;
	};
	uint8_t _raw;
} __packed ata_status_reg_t;

typedef struct {
	uint8_t zero0: 1;
	uint8_t nIEN: 1;
	uint8_t SRST: 1;
	uint8_t zero1: 1;
	uint8_t zero2: 1;
	uint8_t zero3: 1;
	uint8_t zero4: 1;
	uint8_t HOB: 1;
} __packed ata_dev_control_reg_t;


typedef struct {
    struct {
        uint16_t reserved_1 : 1;
        uint16_t retired_3 : 1;
        uint16_t response_incomplete : 1;
        uint16_t retired_2 : 3;
        uint16_t fixed_device : 1;
        uint16_t removable_media : 1;
        uint16_t retired_1 : 7;
        uint16_t device_type : 1;
    } general_configuration;
    
    uint16_t num_cylinders;
    uint16_t specific_configuration;
    uint16_t num_heads;
    uint16_t retired_1[2];
    uint16_t num_sectors_per_track;
    uint16_t vendor_unique_1[3];
    uint8_t  serial_number[20];
    uint16_t retired_2[2];
    uint16_t obsolete_1;
    uint8_t  firmware_revision[8];
    uint8_t  model_number[40];
    uint8_t  maximum_block_transfer;
    uint8_t  vendor_unique_2;
    
    struct {
        uint16_t feature_supported : 1;
        uint16_t reserved : 15;
    } trusted_computing;
    
    struct {
        uint8_t  current_long_physical_sector_alignment : 2;
        uint8_t  reserved_byte_49 : 6;
        uint8_t  dma_supported : 1;
        uint8_t  lba_supported : 1;
        uint8_t  iordy_disable : 1;
        uint8_t  iordy_supported : 1;
        uint8_t  reserved_1 : 1;
        uint8_t  standby_timer_support : 1;
        uint8_t  reserved_2 : 2;
        uint16_t reserved_word_50;
    } capabilities;
    
    uint16_t obsolete_words_51[2];
    uint16_t translation_fields_valid : 3;
    uint16_t reserved_3 : 5;
    uint16_t free_fall_control_sensitivity : 8;
    uint16_t number_of_current_cylinders;
    uint16_t number_of_current_heads;
    uint16_t current_sectors_per_track;
    uint32_t current_sector_capacity;
    uint8_t  current_multi_sector_setting;
    uint8_t  multi_sector_setting_valid : 1;
    uint8_t  reserved_byte_59 : 3;
    uint8_t  sanitize_feature_supported : 1;
    uint8_t  crypto_scramble_ext_command_supported : 1;
    uint8_t  overwrite_ext_command_supported : 1;
    uint8_t  block_erase_ext_command_supported : 1;
    uint32_t user_addressable_sectors;
    uint16_t obsolete_word_62;
    uint16_t multi_word_dma_support : 8;
    uint16_t multi_word_dma_active : 8;
    uint16_t advanced_pio_modes : 8;
    uint16_t reserved_byte_64 : 8;
    uint16_t minimum_mwxfer_cycle_time;
    uint16_t recommended_mwxfer_cycle_time;
    uint16_t minimum_pio_cycle_time;
    uint16_t minimum_pio_cycle_time_iordy;
    
    struct {
        uint16_t zoned_capabilities : 2;
        uint16_t non_volatile_write_cache : 1;
        uint16_t extended_user_addressable_sectors_supported : 1;
        uint16_t device_encrypts_all_user_data : 1;
        uint16_t read_zero_after_trim_supported : 1;
        uint16_t optional_28bit_commands_supported : 1;
        uint16_t ieee_1667 : 1;
        uint16_t download_microcode_dma_supported : 1;
        uint16_t set_max_set_password_unlock_dma_supported : 1;
        uint16_t write_buffer_dma_supported : 1;
        uint16_t read_buffer_dma_supported : 1;
        uint16_t device_config_identify_set_dma_supported : 1;
        uint16_t lpsaer_c_supported : 1;
        uint16_t deterministic_read_after_trim_supported : 1;
        uint16_t c_fast_spec_supported : 1;
    } additional_supported;
    
    uint16_t reserved_words_70[5];
    uint16_t queue_depth : 5;
    uint16_t reserved_word_75 : 11;
    
    struct {
        uint16_t reserved_0 : 1;
        uint16_t sata_gen_1 : 1;
        uint16_t sata_gen_2 : 1;
        uint16_t sata_gen_3 : 1;
        uint16_t reserved_1 : 4;
        uint16_t ncq : 1;
        uint16_t hipm : 1;
        uint16_t phy_events : 1;
        uint16_t ncq_unload : 1;
        uint16_t ncq_priority : 1;
        uint16_t host_auto_ps : 1;
        uint16_t device_auto_ps : 1;
        uint16_t read_log_dma : 1;
        uint16_t reserved_2 : 1;
        uint16_t current_speed : 3;
        uint16_t ncq_streaming : 1;
        uint16_t ncq_queue_mgmt : 1;
        uint16_t ncq_receive_send : 1;
        uint16_t devslp_to_reduced_pwr_state : 1;
        uint16_t reserved_3 : 8;
    } serial_ata_capabilities;
    
    struct {
        uint16_t reserved_0 : 1;
        uint16_t non_zero_offsets : 1;
        uint16_t dma_setup_auto_activate : 1;
        uint16_t dipm : 1;
        uint16_t in_order_data : 1;
        uint16_t hardware_feature_control : 1;
        uint16_t software_settings_preservation : 1;
        uint16_t ncq_autosense : 1;
        uint16_t devslp : 1;
        uint16_t hybrid_information : 1;
        uint16_t reserved_1 : 6;
    } serial_ata_features_supported;
    
    struct {
        uint16_t reserved_0 : 1;
        uint16_t non_zero_offsets : 1;
        uint16_t dma_setup_auto_activate : 1;
        uint16_t dipm : 1;
        uint16_t in_order_data : 1;
        uint16_t hardware_feature_control : 1;
        uint16_t software_settings_preservation : 1;
        uint16_t device_auto_ps : 1;
        uint16_t devslp : 1;
        uint16_t hybrid_information : 1;
        uint16_t reserved_1 : 6;
    } serial_ata_features_enabled;
    
    uint16_t major_revision;
    uint16_t minor_revision;
    
    struct {
        uint16_t smart_commands : 1;
        uint16_t security_mode : 1;
        uint16_t removable_media_feature : 1;
        uint16_t power_management : 1;
        uint16_t reserved_1 : 1;
        uint16_t write_cache : 1;
        uint16_t look_ahead : 1;
        uint16_t release_interrupt : 1;
        uint16_t service_interrupt : 1;
        uint16_t device_reset : 1;
        uint16_t host_protected_area : 1;
        uint16_t obsolete_1 : 1;
        uint16_t write_buffer : 1;
        uint16_t read_buffer : 1;
        uint16_t nop : 1;
        uint16_t obsolete_2 : 1;
        uint16_t download_microcode : 1;
        uint16_t dma_queued : 1;
        uint16_t cfa : 1;
        uint16_t advanced_pm : 1;
        uint16_t msn : 1;
        uint16_t power_up_in_standby : 1;
        uint16_t manual_power_up : 1;
        uint16_t reserved_2 : 1;
        uint16_t set_max : 1;
        uint16_t acoustics : 1;
        uint16_t big_lba : 1;
        uint16_t device_config_overlay : 1;
        uint16_t flush_cache : 1;
        uint16_t flush_cache_ext : 1;
        uint16_t word_valid_83 : 2;
        uint16_t smart_error_log : 1;
        uint16_t smart_self_test : 1;
        uint16_t media_serial_number : 1;
        uint16_t media_card_pass_through : 1;
        uint16_t streaming_feature : 1;
        uint16_t gp_logging : 1;
        uint16_t write_fua : 1;
        uint16_t write_queued_fua : 1;
        uint16_t wwn_64bit : 1;
        uint16_t urg_read_stream : 1;
        uint16_t urg_write_stream : 1;
        uint16_t reserved_for_tech_report : 2;
        uint16_t idle_with_unload_feature : 1;
        uint16_t word_valid : 2;
    } command_set_support;
    
    struct {
        uint16_t smart_commands : 1;
        uint16_t security_mode : 1;
        uint16_t removable_media_feature : 1;
        uint16_t power_management : 1;
        uint16_t reserved_1 : 1;
        uint16_t write_cache : 1;
        uint16_t look_ahead : 1;
        uint16_t release_interrupt : 1;
        uint16_t service_interrupt : 1;
        uint16_t device_reset : 1;
        uint16_t host_protected_area : 1;
        uint16_t obsolete_1 : 1;
        uint16_t write_buffer : 1;
        uint16_t read_buffer : 1;
        uint16_t nop : 1;
        uint16_t obsolete_2 : 1;
        uint16_t download_microcode : 1;
        uint16_t dma_queued : 1;
        uint16_t cfa : 1;
        uint16_t advanced_pm : 1;
        uint16_t msn : 1;
        uint16_t power_up_in_standby : 1;
        uint16_t manual_power_up : 1;
        uint16_t reserved_2 : 1;
        uint16_t set_max : 1;
        uint16_t acoustics : 1;
        uint16_t big_lba : 1;
        uint16_t device_config_overlay : 1;
        uint16_t flush_cache : 1;
        uint16_t flush_cache_ext : 1;
        uint16_t resrved_3 : 1;
        uint16_t words_119_120_valid : 1;
        uint16_t smart_error_log : 1;
        uint16_t smart_self_test : 1;
        uint16_t media_serial_number : 1;
        uint16_t media_card_pass_through : 1;
        uint16_t streaming_feature : 1;
        uint16_t gp_logging : 1;
        uint16_t write_fua : 1;
        uint16_t write_queued_fua : 1;
        uint16_t wwn_64bit : 1;
        uint16_t urg_read_stream : 1;
        uint16_t urg_write_stream : 1;
        uint16_t reserved_for_tech_report : 2;
        uint16_t idle_with_unload_feature : 1;
        uint16_t reserved_4 : 2;
    } command_set_active;
    
    uint16_t ultra_dma_support : 8;
    uint16_t ultra_dma_active : 8;
    
    struct {
        uint16_t time_required : 15;
        uint16_t extended_time_reported : 1;
    } normal_security_erase_unit;
    
    struct {
        uint16_t time_required : 15;
        uint16_t extended_time_reported : 1;
    } enhanced_security_erase_unit;
    
    uint16_t current_apm_level : 8;
    uint16_t reserved_word_91 : 8;
    uint16_t master_password_id;
    uint16_t hardware_reset_result;
    uint16_t current_acoustic_value : 8;
    uint16_t recommended_acoustic_value : 8;
    uint16_t stream_min_request_size;
    uint16_t streaming_transfer_time_dma;
    uint16_t streaming_access_latency_dma_pio;
    uint32_t streaming_perf_granularity;
    uint32_t max_48bit_lba[2];
    uint16_t streaming_transfer_time;
    uint16_t dsm_cap;
    
    struct {
        uint16_t logical_sectors_per_physical_sector : 4;
        uint16_t reserved_0 : 8;
        uint16_t logical_sector_longer_than_256_words : 1;
        uint16_t multiple_logical_sectors_per_physical_sector : 1;
        uint16_t reserved_1 : 2;
    } physical_logical_sector_size;
    
    uint16_t inter_seek_delay;
    uint16_t world_wide_name[4];
    uint16_t reserved_for_world_wide_name_128[4];
    uint16_t reserved_for_tlc_technical_report;
    uint16_t words_per_logical_sector[2];
    struct {
        uint16_t reserved_for_drq_technical_report : 1;
        uint16_t write_read_verify : 1;
        uint16_t write_uncorrectable_ext : 1;
        uint16_t read_write_log_dma_ext : 1;
        uint16_t download_microcode_mode_3 : 1;
        uint16_t freefall_control : 1;
        uint16_t sense_data_reporting : 1;
        uint16_t extended_power_conditions : 1;
        uint16_t reserved_0 : 6;
        uint16_t word_valid : 2;
    } command_set_support_ext;
    
    struct {
        uint16_t reserved_for_drq_technical_report : 1;
        uint16_t write_read_verify : 1;
        uint16_t write_uncorrectable_ext : 1;
        uint16_t read_write_log_dma_ext : 1;
        uint16_t download_microcode_mode_3 : 1;
        uint16_t freefall_control : 1;
        uint16_t sense_data_reporting : 1;
        uint16_t extended_power_conditions : 1;
        uint16_t reserved_0 : 6;
        uint16_t reserved_1 : 2;
    } command_set_active_ext;
    
    uint16_t reserved_for_expanded_support_and_active[6];
    uint16_t msn_support : 2;
    uint16_t reserved_word_127 : 14;
    
    struct {
        uint16_t security_supported : 1;
        uint16_t security_enabled : 1;
        uint16_t security_locked : 1;
        uint16_t security_frozen : 1;
        uint16_t security_count_expired : 1;
        uint16_t enhanced_security_erase_supported : 1;
        uint16_t reserved_0 : 2;
        uint16_t security_level : 1;
        uint16_t reserved_1 : 7;
    } security_status;
    
    uint16_t reserved_word_129[31];
    
    struct {
        uint16_t maximum_current_in_ma : 12;
        uint16_t cfa_power_mode_1_disabled : 1;
        uint16_t cfa_power_mode_1_required : 1;
        uint16_t reserved_0 : 1;
        uint16_t word_160_supported : 1;
    } cfa_power_mode_1;
    
    uint16_t reserved_for_cfa_word_161[7];
    uint16_t nominal_form_factor : 4;
    uint16_t reserved_word_168 : 12;
    
    struct {
        uint16_t supports_trim : 1;
        uint16_t reserved_0 : 15;
    } data_set_management_feature;
    
    uint16_t additional_product_id[4];
    uint16_t reserved_for_cfa_word_174[2];
    uint16_t current_media_serial_number[30];
    
    struct {
        uint16_t supported : 1;
        uint16_t reserved_0 : 1;
        uint16_t write_same_suported : 1;
        uint16_t error_recovery_control_supported : 1;
        uint16_t feature_control_suported : 1;
        uint16_t data_tables_suported : 1;
        uint16_t reserved_1 : 6;
        uint16_t vendor_specific : 4;
    } sct_command_transport;
    
    uint16_t reserved_word_207[2];
    
    struct {
        uint16_t alignment_of_logical_within_physical : 14;
        uint16_t word_209_supported : 1;
        uint16_t reserved_0 : 1;
    } block_alignment;
    
    uint16_t write_read_verify_sector_count_mode_3_only[2];
    uint16_t write_read_verify_sector_count_mode_2_only[2];
    
    struct {
        uint16_t nv_cache_power_mode_enabled : 1;
        uint16_t reserved_0 : 3;
        uint16_t nv_cache_feature_set_enabled : 1;
        uint16_t reserved_1 : 3;
        uint16_t nv_cache_power_mode_version : 4;
        uint16_t nv_cache_feature_set_version : 4;
    } nv_cache_capabilities;
    
    uint16_t nv_cache_size_lsw;
    uint16_t nv_cache_size_msw;
    uint16_t nominal_media_rotation_rate;
    uint16_t reserved_word_218;
    
    struct {
        uint8_t  nv_cache_estimated_time_to_spin_up_in_seconds;
        uint8_t  reserved;
    } nv_cache_options;
    
    uint16_t write_read_verify_sector_count_mode : 8;
    uint16_t reserved_word_220 : 8;
    uint16_t reserved_word_221;
    
    struct {
        uint16_t major_version : 12;
        uint16_t transport_type : 4;
    } transport_major_version;
    
    uint16_t transport_minor_version;
    uint16_t reserved_word_224[6];
    uint32_t extended_number_of_user_addressable_sectors[2];
    uint16_t min_blocks_per_download_microcode_mode_03;
    uint16_t max_blocks_per_download_microcode_mode_03;
    uint16_t reserved_word_236[19];
    uint16_t signature : 8;
    uint16_t check_sum : 8;
} __packed ata_identify_device_data_t;

typedef struct {
    uint32_t id;
    char name[41];
} ata_drive_t;

enum {
//Address mark not found.
	ATA_ERR_AMNF,
//Track zero not found.
	ATA_ERR_TKZNF,
//Aborted command.
	ATA_ERR_ABRT,
//Media change request.
	ATA_ERR_MCR,
//ID not found.
	ATA_ERR_IDNF,
//Media changed.
	ATA_ERR_MC,
//Uncorrectable data error.
	ATA_ERR_UNC,
//Bad Block detected.
	ATA_ERR_BBK,
};

void init_ata();

#endif